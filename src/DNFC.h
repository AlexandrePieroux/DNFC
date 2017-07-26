#ifndef _DNFCH_
#define _DNFCH_

#include <stdio.h>
#include <stdbool.h>

#include "classifiers/static_classifiers/static_classifiers.h"
#include "classifiers/dynamic_classifiers/dynamic_classifiers.h"
#include "../libs/queue/queue.h"

typedef unsigned char u_char; // Defining u_char type for convenient display

struct DNFC_action
{
   struct queue* pckt_queue;
   flow_table* flow_table;
};

struct DNFC_pckt
{
   u_char* data;
   size_t size;
};

struct DNFC_tag
{
   struct linked_list* flow_pckt_list;
   struct hazard_pointer* flow_pckt_list_hps;
};

struct DNFC_tagged_pckt
{
   struct DNFC_tag* tag;
   struct DNFC_pckt* pckt;
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

bool DNFC_process(struct DNFC* classifier, u_char* pckt, size_t pckt_length);

struct queue* DNFC_get_rule_queue(struct classifier_rule* rule);

void DNFC_free_tag(void* tag);

//void free_DNFC(struct DNFC* classifier);

#endif
