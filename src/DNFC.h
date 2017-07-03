#ifndef _DNFCH_
#define _DNFCH_

#include <stdio.h>
#include <stdbool.h>

#include "classifiers/static_classifiers/static_classifiers.h"
#include "../libs/queue/queue.h"

/* Supported protocols headers */

#include "protocols/TCP/TCP.h"

typedef unsigned char u_char; // Defining u_char type for convenient display

struct tuple
{
   void* a;
   void* b;
};

struct DNFC
{
   struct hypercuts_classifier* static_classifier;
   void (*callback)(u_char*, size_t);
   size_t queue_limit;
   size_t nb_thread;
};


struct DNFC* new_DNFC(size_t nb_threads,
                      struct classifier_rule ***rules,
                      size_t nb_rules,
                      size_t queue_limit,
                      void (*callback)(u_char*, size_t),
                      bool verbose);

void DNFC_process(struct DNFC* classifier, u_char* pckt, size_t pckt_length);

//void free_DNFC(struct DNFC* classifier);

#endif
