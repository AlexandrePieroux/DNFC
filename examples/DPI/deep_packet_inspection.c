#include <stdio.h>
#include <stdbool.h>

#include "../../libs/hash_table/hash_table.h"
#include "../../libs/thread_pool/thread_pool.h"
#include "../../src/DNFC.h"

#define NB_THREADS   64


struct arguments_t
{
   struct DNFC* classifier;
   u_char* pckt;
   size_t pckt_length;
};


int main (int argc, char **argv)
{
   struct threadpool_t* pool = new_threadpool(NB_THREADS);
   return 0;
}


void job_classify(void* args)
{
   struct arguments_t* args_cast = (struct arguments_t*) args;
   DNFC_process(args_cast->classifier, args_cast->pckt, args_cast->pckt_length);
}
