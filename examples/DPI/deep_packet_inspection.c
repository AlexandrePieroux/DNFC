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
   // Should include netmap ring
};


void put_host_stack(u_char* pckt, size_t pckt_size);

void* job_classify(void* args);

void* job_inspection(void* args);


/*
 * how many packets on this set of queues ?
 */
int pkt_queued(struct nm_desc *d, int tx)
{
   uint32_t i, tot = 0;
   
   if (tx)
   {
      for (i = d->first_tx_ring; i <= d->last_tx_ring; i++)
         tot += nm_ring_space(NETMAP_TXRING(d->nifp, i));
   } else {
      for (i = d->first_rx_ring; i <= d->last_rx_ring; i++)
         tot += nm_ring_space(NETMAP_RXRING(d->nifp, i));
   }
   return tot;
}

int main (int argc, char **argv)
{
   struct threadpool_t* classifiers_pool = new_threadpool(NB_THREADS);
   struct threadpool_t* inspectors_pool = new_threadpool(NB_THREADS);
   struct classifier_rule** rules;
   uint32_t nb_rules = 1;
   
   struct DNFC* classifier = new_DNFC(NB_THREADS, &rules, nb_rules, QUEUE_THRESHOLD, put_host_stack, false);
   for (uint32_t i = 0; i < NB_THREADS; ++i)
   {
      struct args_inspection_t* args_ins = chkmalloc(sizeof *args_ins);
      args_ins->queue = (struct queue*)rules[0]->action;
      threadpool_add_work(inspectors_pool, job_inspection, args_ins);
   }
   
   struct pollfd pollfd;
   int ch;
   uint32_t burst = 1024;
   uint32_t wait_link = 4;
   struct nm_desc *pa = NULL;
   char *ifa = NULL;
   char ifabuf[64] = {0};
   int loopback = 0;
   
   pa = nm_open(ifa, NULL, 0, NULL);
   if (pa == NULL)
   {
      D("cannot open %s", ifa);
      return (1);
   }
   
   uint32_t zerocopy = pa->mem;
   D("------- zerocopy %s supported", zerocopy ? "" : "NOT ");
   
   /* setup poll(2) array */
   memset(&pollfd, 0, sizeof(pollfd));
   pollfd.fd = pa->fd;
   
   D("Wait %d secs for link to come up...", wait_link);
   sleep(wait_link);
   D("Ready to go, %s 0x%x/%d.", pa->req.nr_name, pa->first_rx_ring, pa->req.nr_rx_rings);
   
   /* main loop */
   signal(SIGINT, sigint_h);
   while (!do_abort)
   {
      int n0;
      int ret;
      pollfd.events = 0;
      pollfd.revents = 0;
      
      n0 = pkt_queued(pa, 0);

      if (n0)
         pollfd[1].events |= POLLOUT;
      else
         pollfd[0].events |= POLLIN;
      
      
      /* poll() also cause kernel to txsync/rxsync the NICs */
      ret = poll(pollfd, 1, 2500);
      if (ret < 0)
         continue;
      
      if (pollfd.revents & POLLERR)
      {
         struct netmap_ring *rx = NETMAP_RXRING(pa->nifp, pa->cur_rx_ring);
         D("error on fd0, rx [%d,%d,%d)",
           rx->head, rx->cur, rx->tail);
      }
      
      if (pollfd.revents & POLLOUT)
         move(pb, pa, burst);
      
   }
   nm_close(pb);
   nm_close(pa);
   
   return 0;
}

void put_host_stack(u_char* pckt, size_t pckt_size)
{
   
}

void* job_classify(void* args)
{
   struct args_classification_t* args_cast = (struct args_classification_t*) args;
   DNFC_process(args_cast->classifier, args_cast->pckt, args_cast->pckt_length);
   return NULL;
}


void* job_inspection(void* args)
{
   return NULL;
}
