#include <stdio.h>
#include <stdbool.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>
#include <sys/poll.h>

#include "../../libs/hash_table/hash_table.h"
#include "../../libs/thread_pool/thread_pool.h"
#include "../../src/DNFC.h"

#define NB_THREADS   64
#define QUEUE_THRESHOLD   4096

struct args_classification_t
{
   struct DNFC* classifier;
   u_char* pckt;
   size_t pckt_length;
};

struct args_inspection_t
{
   struct queue* queue;
};

struct queue* ifa_default_queue;
struct queue* ifb_default_queue;

int verbose = 0;
static int do_abort = 0;



/*       Private Function       */

void* job_classify(void* args)
{
   struct args_classification_t* args_cast = (struct args_classification_t*) args;
   DNFC_process(args_cast->classifier, args_cast->pckt, args_cast->pckt_length);
   return NULL;
}

void put_default_ifa_queue(u_char* pckt, size_t pckt_lenght)
{
   queue_push(ifa_default_queue, pckt);
}

void put_default_ifb_queue(u_char* pckt, size_t pckt_lenght)
{
   queue_push(ifb_default_queue, pckt);
}

static void sigint_h(int sig)
{
   (void)sig;	/* UNUSED */
   do_abort = 1;
   signal(SIGINT, SIG_DFL);
}


/*
 * how many packets on this set of queues ?
 */
int pkt_queued(struct nm_desc *d, int tx)
{
   uint32_t i;
   uint32_t tot = 0;
   
   if(tx){
      for (i = d->first_tx_ring; i <= d->last_tx_ring; i++)
         tot += nm_ring_space(NETMAP_TXRING(d->nifp, i));
   } else {
      for (i = d->first_rx_ring; i <= d->last_rx_ring; i++)
         tot += nm_ring_space(NETMAP_RXRING(d->nifp, i));
   }
   return tot;
}

/*
 * move up to 'limit' pkts from rxring to txring swapping buffers.
 */
static int process_rings(struct netmap_ring *rxring,
                         struct netmap_ring *txring,
                         uint32_t limit)
{
   uint32_t j;
   uint32_t k;
   uint32_t m = 0;
   
   /* print a warning if any of the ring flags is set (e.g. NM_REINIT) */
   if (rxring->flags || txring->flags)
      D("rxflags %x txflags %x", rxring->flags, txring->flags);
   
   // Setting cursors to the current positions of the rings
   j = rxring->cur; /* RX */
   k = txring->cur; /* TX */
   
   // Take the smaller ring space (to avoid overflow)
   m = nm_ring_space(rxring);
   if (m < limit)
      limit = m;
   m = nm_ring_space(txring);
   if (m < limit)
      limit = m;
   m = limit;
   
   // While we still have space on the smaller ring
   while (limit-- > 0) {
      struct netmap_slot *rs = &rxring->slot[j];
      struct netmap_slot *ts = &txring->slot[k];
      
      /* swap packets */
      if (ts->buf_idx < 2 || rs->buf_idx < 2) {
         D("wrong index rx[%d] = %d  -> tx[%d] = %d", j, rs->buf_idx, k, ts->buf_idx);
         sleep(2);
      }
      
      /* copy the packet length. */
      if (rs->len > 2048) {
         D("wrong len %d rx[%d] -> tx[%d]", rs->len, j, k);
         rs->len = 0;
      }
      
      ts->len = rs->len;
      char *rxbuf = NETMAP_BUF(rxring, rs->buf_idx);
      char *txbuf = NETMAP_BUF(txring, ts->buf_idx);
      nm_pkt_copy(rxbuf, txbuf, ts->len);

      // Next slot in the ring
      j = nm_ring_next(rxring, j);
      k = nm_ring_next(txring, k);
   }
   
   // Advance curent pointer and the head pointer to the last processed slot
   rxring->head = rxring->cur = j;
   txring->head = txring->cur = k;
   
   return (m);
}

/* move packts from src to destination */
static int move(struct nm_desc *src, struct nm_desc *dst, uint32_t limit)
{
   struct netmap_ring *txring;
   struct netmap_ring *rxring;
   uint32_t m = 0;
   uint32_t si = src->first_rx_ring;
   uint32_t di = dst->first_tx_ring;
   
   while (si <= src->last_rx_ring && di <= dst->last_tx_ring) {
      rxring = NETMAP_RXRING(src->nifp, si);
      txring = NETMAP_TXRING(dst->nifp, di);
      if (nm_ring_empty(rxring)) {
         si++;
         continue;
      }
      if (nm_ring_empty(txring)) {
         di++;
         continue;
      }
      m += process_rings(rxring, txring, limit);
   }
   
   return (m);
}


/*
 * bridge [-v] if1 [if2]
 *
 * If only one name, or the two interfaces are the same,
 * bridges userland and the adapter. Otherwise bridge
 * two intefaces.
 */
int main(int argc, char **argv)
{
   struct pollfd pollfd[2];
   int ch;
   uint32_t burst = 1024;
   uint32_t wait_link = 4;
   char* ifa = NULL;
   char* ifb = NULL;
   char ifabuf[64] = {0};
   
   // One classifier per interface
   /*
    This could be simplified in one classifier for both.
    But this would require to check on which interface the packet
    come from to branch it to the other interface. As the number
    of rules is low the memory overhead is not significant.
    */
   ifa_default_queue = new_queue(QUEUE_THRESHOLD, NB_THREADS);
   size_t nb_ifa_rules = 1;
   struct classifier_rule* ifa_rule = chkmalloc(sizeof(*ifa_rule) * nb_ifa_rules);
   ifa_rule->fields = chkmalloc(sizeof(ifa->fields));
   ifa_rule->fields[0]->bit_length = 16;
   ifa_rule->fields[0]->mask = 255;
   ifa_rule->fields[0]->offset = 264;
   ifa_rule->fields[0]->value = 80;

   ifb_default_queue = new_queue(QUEUE_THRESHOLD, NB_THREADS);
   size_t nb_ifb_rules = 1;
   struct classifier_rule* ifb_rule = chkmalloc(sizeof(*ifb_rule) * nb_ifb_rules);
   ifb_rule->fields = chkmalloc(sizeof(ifb->fields));
   ifb_rule->fields[0]->bit_length = 16;
   ifb_rule->fields[0]->mask = 255;
   ifb_rule->fields[0]->offset = 264;
   ifb_rule->fields[0]->value = 80;
   
   struct DNFC* ifa_classifier = new_DNFC(NB_THREADS,
                                          ifa_rule,
                                          nb_ifa_rules,
                                          QUEUE_THRESHOLD,
                                          put_default_ifa_queue,
                                          verbose);
   struct DNFC* ifa_classifier = new_DNFC(NB_THREADS,
                                          ifb_rule,
                                          nb_ifb_rules,
                                          QUEUE_THRESHOLD,
                                          put_default_ifb_queue,
                                          verbose);
   
   // Open interface a
   struct nm_desc *pa = nm_open(ifa, NULL, 0, NULL);
   if (!pa) {
      D("cannot open %s", ifa);
      return (1);
   }
   
   // Open interface b and trying to reuse same mmaped region of pa
   struct nm_desc *pb = nm_open(ifb, NULL, NM_OPEN_NO_MMAP, pa);
   if (!pb) {
      D("cannot open %s", ifb);
      nm_close(pa);
      return (1);
   }
   
   /* setup poll(2) variables. */
   memset(pollfd, 0, sizeof(pollfd));
   pollfd[0].fd = pa->fd;
   pollfd[1].fd = pb->fd;
   
   // Waiting for the link to be up
   sleep(wait_link);
   
   /* main loop */
   signal(SIGINT, sigint_h);
   while (!do_abort) {
      
      // Setting variables
      pollfd[0].events = 0;
      pollfd[1].events = 0;
      
      pollfd[0].revents = 0;
      pollfd[1].revents = 0;
      
      // Checking if there are space in the queues
      int n0 = pkt_queued(pa, 0);
      int n1 = pkt_queued(pb, 0);
      
      //Set fd event write if we have space in the queue
      if (n0)
         pollfd[1].events |= POLLOUT;
      else
         pollfd[0].events |= POLLIN;
      
      if (n1)
         pollfd[0].events |= POLLOUT;
      else
         pollfd[1].events |= POLLIN;
      
      // Poll for events
      int ret = poll(pollfd, 2, 2500);
      if (ret < 0)
         continue;
      
      // Check for errors
      if (pollfd[0].revents & POLLERR) {
         struct netmap_ring *rx = NETMAP_RXRING(pa->nifp, pa->cur_rx_ring);
         D("error on fd0, rx [%d,%d,%d)",
           rx->head, rx->cur, rx->tail);
      }
      if (pollfd[1].revents & POLLERR) {
         struct netmap_ring *rx = NETMAP_RXRING(pb->nifp, pb->cur_rx_ring);
         D("error on fd1, rx [%d,%d,%d)",
           rx->head, rx->cur, rx->tail);
      }
      
      // Move packet from an interface to another
      if (pollfd[0].revents & POLLOUT) {
         move(pb, pa, burst);
      }
      if (pollfd[1].revents & POLLOUT) {
         move(pa, pb, burst);
      }
   }
   nm_close(pb);
   nm_close(pa);
   
   return (0);
}
