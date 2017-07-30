#include <stdio.h>
#include <stdbool.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>
#include <sys/poll.h>

#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

#include "../../libs/hash_table/hash_table.h"
#include "../../libs/thread_pool/thread_pool.h"
#include "../../src/DNFC.h"

#define NB_THREADS   64
#define QUEUE_THRESHOLD   4096

typedef unsigned char u_char;

/*        Private variables       */
// Structure used to store arguments for the classifiction of a packet
struct args_classification_t
{
   struct DNFC* classifier;
   u_char* pckt;
   size_t pckt_length;
   struct nm_desc* stack;
};

// Structure used to store arguments for the inspection of a packet
struct args_inspection_t
{
   struct queue* queue;
};

// Configuration variables
int verbose = 0;
static int do_abort = 0;

// Default queue
struct queue* default_pa_q;
struct queue* default_phost_q;

// Interfaces descriptors
struct nm_desc* pa;
struct nm_desc* phost;
/*       Private variables       */



/*       Private Function       */
// Print program usage
void usage();

// Retrive the set of rules used for the classifier
struct classifier_rule** get_rule_set();

// Put available packets in the descriptor in the classifier until 'limit' is reached
static int move_pckt_to_classifier(struct nm_desc *src,
                                   uint32_t limit,
                                   struct DNFC* classifier,
                                   struct threadpool_t* classify_pool);

// Move available packets in the queue in the destination interface until 'limit' is reached
static int pckt_from_queue_iface(struct nm_desc *dst,
                                 uint32_t limit,
                                 struct queue* queue,
                                 void (*pckt_process)(char**, size_t*, struct queue*));

// Process tagged packets
void tagged_pckt_process(char** pckt, size_t* size, struct queue* queue);

// Process non-tagged packets
void non_tagged_pckt_process(char** pckt, size_t* size, struct queue* queue);

// Return the number of slots available in each transmission rings of the decriptor if tx is superior 1, otherwise
// it return the number of slots available in each transmission rings of the decriptor.
int pkt_queued(struct nm_desc *d, int tx);

// Classification job method:
// Use an instantiated classifier to classify a given packet
void* job_classify(void* args);

void default_action(u_char* pckt, size_t pckt_length);

// Interupt function
static void sigint_h();
/*       Private Function       */



/*
 * bridge [-v] if1 [if2]
 */
int main(int argc, char **argv)
{
   struct pollfd pollfd[2];
   int ch;
   uint32_t burst = 1024;
   uint32_t wait_link = 4;
   char* ifa = NULL;
   
   while ((ch = getopt(argc, argv, "hb:i:vw")) != -1) {
      switch (ch) {
         default:
            D("bad option %c %s", ch, optarg);
            /* fallthrough */
         case 'h':
            usage();
            break;
         case 'b':	/* burst */
            burst = atoi(optarg);
            break;
         case 'i':	/* interface */
            if (ifa == NULL)
               ifa = optarg;
            else
               D("%s ignored, already have an interfaces",
                 optarg);
            break;
         case 'v':
            verbose++;
            break;
         case 'w':
            wait_link = atoi(optarg);
            break;
      }
      
   }
   
   // Branch to the host stack
   char host_buff[64] = {0};
   snprintf(host_buff, sizeof(host_buff) - 1, "%s^", ifa);
   char* host_stack = host_buff;
   
   size_t nb_rules = 2;
   struct classifier_rule** rules = get_rule_set();
   
   // Initiate the classifier with the rules
   struct DNFC* classifier = new_DNFC(NB_THREADS,
                                      &rules,
                                      nb_rules,
                                      QUEUE_THRESHOLD,
                                      NULL,
                                      verbose);
   
   // Init the default queue that receive the packets not matched
   default_phost_q = new_queue(QUEUE_THRESHOLD, NB_THREADS);
   default_pa_q = new_queue(QUEUE_THRESHOLD, NB_THREADS);

   // We instantiate the pool of threads that will perform the classification
   struct threadpool_t* pool = new_threadpool(NB_THREADS);
   
   // Open interface host stack
   phost = nm_open(host_stack, NULL, 0, NULL);
   if (!phost) {
      D("cannot open %s", host_stack);
      return (1);
   }
   
   // Open interface and try to reuse same mmaped region of phost
   pa = nm_open(ifa, NULL, NM_OPEN_NO_MMAP, phost);
   if (!pa) {
      D("cannot open %s", ifa);
      nm_close(pa);
      return (1);
   }
   
   /* setup poll(2) variables. */
   memset(pollfd, 0, sizeof(pollfd));
   pollfd[0].fd = phost->fd;
   pollfd[1].fd = pa->fd;
   
   // Waiting for the link to be up
   D("Wait %d secs for link to come up...", wait_link);
   sleep(wait_link);
   D("Ready to go\n");
   
   /* main loop */
   signal(SIGINT, sigint_h);
   while (!do_abort) {
      
      // Setting variables
      pollfd[0].events = 0;
      pollfd[1].events = 0;
      
      pollfd[0].revents = 0;
      pollfd[1].revents = 0;
      
      // Checking if there are space in the queues
      int n0 = pkt_queued(phost, 0);
      int n1 = pkt_queued(pa, 0);
      
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
         struct netmap_ring *rx = NETMAP_RXRING(phost->nifp, phost->cur_rx_ring);
         D("error on fd0, rx [%d,%d,%d)",
           rx->head, rx->cur, rx->tail);
      }
      if (pollfd[1].revents & POLLERR) {
         struct netmap_ring *rx = NETMAP_RXRING(pa->nifp, pa->cur_rx_ring);
         D("error on fd1, rx [%d,%d,%d)",
           rx->head, rx->cur, rx->tail);
      }
      
      // Move packet from interface/host stack into the classifier
      if (pollfd[0].revents & POLLIN)
         move_pckt_to_classifier(phost, burst, classifier, pool);
      if (pollfd[1].revents & POLLIN)
         move_pckt_to_classifier(pa, burst, classifier, pool);
      
      
      // Move packets from the classifier to the interface/host stack
      if (pollfd[0].revents & POLLOUT)
         pckt_from_queue_iface(phost, burst, ((struct DNFC_action*) rules[1]->action)->pckt_queue, tagged_pckt_process);
      if (pollfd[1].revents & POLLOUT)
         pckt_from_queue_iface(pa, burst, ((struct DNFC_action*) rules[0]->action)->pckt_queue, tagged_pckt_process);
      
      
      // Process the default queues
      if (default_phost_q->size != 0)
         pckt_from_queue_iface(pa, burst, default_phost_q, non_tagged_pckt_process);
      if (default_pa_q->size != 0)
         pckt_from_queue_iface(phost, burst, default_pa_q, non_tagged_pckt_process);
   }
   nm_close(phost);
   nm_close(pa);
   
   return (0);
}



/*       Private Function       */
void usage()
{
   fprintf(stderr,
           "DNFC DPI example program: perform basic deep packet inspection between a network interface "
           "and the host stack\n"
           "    usage(1): deep_packet_inspection [-v] [-i if] [-b burst] "
           "[-w wait_time]\n"
           "    usage(2): bridge [-v] [-w wait_time] [-L] "
           "[ifa [ifb [burst]]]\n"
           "\n"
           "    example: deep_packet_inspection -w 10 -i netmap:eth3\n"
           );
   exit(1);
}

struct classifier_rule** get_rule_set()
{
   struct classifier_rule** rules = chkmalloc(sizeof(*rules) * 2);
   
   // First rule
   rules[0] = chkmalloc(sizeof(rules[0]));
   rules[0]->fields = chkmalloc(sizeof(rules[0]->fields) * 2);
   
   // Port 80 destination field
   rules[0]->fields[0]->bit_length = 16;
   rules[0]->fields[0]->mask = 255;
   rules[0]->fields[0]->offset = 272;
   rules[0]->fields[0]->value = 80;
   
   // Seconde rule
   rules[1] = chkmalloc(sizeof(rules[1]));
   rules[1]->fields = chkmalloc(sizeof(rules[2]->fields));
   
   // Port 80 source field
   rules[1]->fields[0]->bit_length = 16;
   rules[1]->fields[0]->mask = 255;
   rules[1]->fields[0]->offset = 288;
   rules[1]->fields[0]->value = 80;
   
   return rules;
}

static int move_pckt_to_classifier(struct nm_desc *src,
                                   uint32_t limit,
                                   struct DNFC* classifier,
                                   struct threadpool_t* classify_pool)
{
   struct netmap_ring *rxring;
   struct netmap_slot *rs;
   uint32_t m = 0;
   uint32_t si = src->first_tx_ring;
   uint32_t k;
   
   // Move packets from rule queue
   while (si <= src->last_rx_ring ) {
      rxring = NETMAP_TXRING(src->nifp, si);
      if (nm_ring_empty(rxring)) {
         si++;
         continue;
      }
      
      // Setting cursors to the current positions of the rings
      k = rxring->cur; /* TX */
      while (limit-- > 0) {
         rs= &rxring->slot[k];
         
         /* swap packets */
         if (rs->buf_idx < 2) {
            D("wrong index rx[%d] = %d", k, rs->buf_idx);
            sleep(2);
         }
         
         // Get the packet length and the buffer to receive it
         if (rs->len > 2048) {
            D("wrong len %d tx[%d]", rs->len, k);
            rs->len = 0;
         }
         char* rxbuf = NETMAP_BUF(rxring, rs->buf_idx);
         
         // Construct the argument for classification
         struct args_classification_t* args = chkmalloc(sizeof(*args));
         args->classifier = classifier;
         args->pckt = (u_char*) rxbuf;
         args->pckt_length = rs->len;
         args->stack = src;
         threadpool_add_work(classify_pool, job_classify, args);
         
         // Next slot in the ring
         k = nm_ring_next(rxring, k);
      }
      rxring->head = rxring->cur = k;
   }
   
   return (m);
}


static int pckt_from_queue_iface(struct nm_desc *dst,
                                 uint32_t limit,
                                 struct queue* queue,
                                 void (*pckt_process)(char**, size_t*, struct queue*))
{
   struct netmap_ring *txring;
   uint32_t m_tot = 0;
   uint32_t di = dst->first_tx_ring;
   
   // Move packets from rule queue
   if(queue) {
      // An atomic read might be necessary for the queue size attribute (depending on the system)
      while (di <= dst->last_tx_ring || queue->size != 0) {
         
         txring = NETMAP_TXRING(dst->nifp, di);
         if (nm_ring_empty(txring)) {
            di++;
            continue;
         }
         
         // Setting cursors to the current positions of the rings
         uint32_t k = txring->cur; /* TX */
         uint32_t m = 0;
         
         /* print a warning if any of the ring flags is set (e.g. NM_REINIT) */
         if (txring->flags)
            D("txflags %x", txring->flags);
         
         // Take the smaller ring space (to avoid overflow)
         m = nm_ring_space(txring);
         if (m < limit)
            limit = m;
         m = limit;
         
         // While we still have space on the ring
         while (limit-- > 0) {
            struct netmap_slot *ts = &txring->slot[k];
            
            /* swap packets */
            if (ts->buf_idx < 2) {
               D("wrong index tx[%d] = %d", k, ts->buf_idx);
               sleep(2);
            }
            
            // Get the buffer to receive it
            if (ts->len > 2048) {
               D("wrong len %d tx[%d]", ts->len, k);
               ts->len = 0;
            }
            char* txbuf = NETMAP_BUF(txring, ts->buf_idx);
            
            size_t pckt_len;
            char* pckt;
            
            // Get the packet and the length
            pckt_process(&pckt, &pckt_len, queue);
            
            // Copy the packet
            nm_pkt_copy(pckt, txbuf, pckt_len);
            
            // Next slot in the ring
            k = nm_ring_next(txring, k);
         }
         
         // Advance curent pointer and the head pointer to the last processed slot
         txring->head = txring->cur = k;
         m_tot += m;
      }
   }
   return (m_tot);
}


void tagged_pckt_process(char** pckt, size_t* size, struct queue* queue)
{
   // Pop a packet from the queue
   struct DNFC_tagged_pckt* txtuple = (struct DNFC_tagged_pckt*)queue_pop(queue);
   if(!txtuple)
      return;
   *pckt = (char*) txtuple->pckt->data;
   
   // Get the packet length
   *size = txtuple->pckt->size;
   
   /*
    * This is where you can process matched packets:
    * The packet is contained in 'txtuple->pckt' which contain the data and the length of the packet
    * The tag is contained in 'txtuple->tag' which contain a concurent lock free linked list and the
    * hazardous pointers necessary to manipulate the list.
    * This grant you the packet that matched the rule and the whole flow context.
    */
   
   // Free the pckt tuple structure
   free(txtuple->pckt);
   
   // Free the tuple structure
   free(txtuple);

   return;
}


void non_tagged_pckt_process(char** pckt, size_t* size, struct queue* queue)
{
   // Pop a packet from the queue
   struct DNFC_pckt* txtuple = (struct DNFC_pckt*)queue_pop(queue);
   if(!txtuple)
      return;
   *pckt = (char*) txtuple->data;
   
   // Get the packet length
   *size = txtuple->size;
   
   /*
    * This is where you can process not matched packets.
    */
   
   return;
}


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


void* job_classify(void* args)
{
   // Classify the packet
   struct args_classification_t* args_cast = (struct args_classification_t*) args;
   if(DNFC_process(args_cast->classifier, args_cast->pckt, args_cast->pckt_length))
      D("A match was found dear!\n");
   else {
      // Put the packet in default queue as it does not match any rules
      struct DNFC_pckt* pckt = chkmalloc(sizeof(*pckt));
      if(args_cast->stack == pa)
         queue_push(default_pa_q, pckt);
      else if(args_cast->stack == phost)
         queue_push(default_phost_q, pckt);
   }
   return NULL;
}


static void sigint_h()
{
   do_abort = 1;
   signal(SIGINT, SIG_DFL);
}
/*       Private Function       */
