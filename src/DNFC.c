#include "DNFC.h"


/*          Private Functions              */

#define tag void
#define new_tag chkmalloc(sizeof(tag))

/* IPV4 fields macro */

#define IPV4_PROTOCOL                     4
#define MIN_HEADER_LENGTH                 120

#define GET_VERSION(pckt)          (pckt[0] & 240)
#define GET_IHL(pckt)              (pckt[0] & 15)
#define GET_PROTOCOL(pckt)          pckt[9]

#define GET_HEADER_LENGTH(pckt)    (GET_IHL(pckt) * 32)


struct DNFC_pckt* get_DNFC_pckt(u_char* pckt,
                                size_t pckt_length);

bool is_supported(u_char* pckt,
                  size_t pckt_size,
                  size_t header_size);

void init_queue(struct queue* queue,
                size_t nb_threads,
                size_t queue_limit);

tag* get_flow_tag(struct DNFC* classifier,
                  u_char* pckt,
                  size_t size,
                  size_t header_size,
                  void* protocol_action);

/*          Private Functions              */



struct DNFC* new_DNFC(size_t nb_threads,
                      struct classifier_rule ***rules,
                      size_t nb_rules,
                      size_t queue_limit,
                      void (*callback)(u_char*, size_t),
                      bool verbose)
{
   // Allocate the structure
   struct DNFC* result = chkmalloc(sizeof(*result));
   for (uint32_t i = 0; i < nb_rules; ++i)
      (*rules)[i]->action = NULL;
   
   // Create the hypercut tree structure for static classification
   result->static_classifier = new_hypercuts_classifier(rules, nb_rules, verbose);
   result->nb_thread = nb_threads;
   result->callback = callback;
   return result;
}



void DNFC_process(struct DNFC* classifier,
                  u_char* pckt,
                  size_t pckt_length)
{
   // Check that the protocol is supported
   uint32_t header_length = GET_HEADER_LENGTH(pckt);
   if(!is_supported(pckt, pckt_length, header_length))
   {
      classifier->callback(pckt, pckt_length);
      return;
   }

   // Search for a match in the static classifier
   struct tuple* action = NULL;
   if(!hypercuts_search(classifier->static_classifier, pckt, header_length, (void**)&action))
   {
      classifier->callback(pckt, pckt_length);
      return;
   }
   
   if(!action)
   {
      action = chkmalloc(sizeof(struct tuple*));
      action->a = new_queue(classifier->queue_limit, classifier->nb_thread);
   }
   
   // Search for a match in the dynamic classifier
   tag* flow_tag = get_flow_tag(classifier, pckt, pckt_length, header_length, action->b);
   
   // We build a pair with the tag and the packet
   struct tuple* packet_result = chkmalloc(sizeof(*packet_result));
   packet_result->a = flow_tag;
   packet_result->b = pckt;
   
   // We push the pair in the queue of the static rule
   queue_push(action->a, packet_result);
}



/*          Private Functions              */

bool is_supported(u_char* pckt,
                  size_t pckt_size,
                  size_t header_size)
{
   // We check the header length
   if((pckt_size <= header_size) || header_size < MIN_HEADER_LENGTH)
      return false;
   
   // We check the version (only support IPV4 for now)
   if(GET_VERSION(pckt) != IPV4_PROTOCOL)
      return false;
   
   // Check the protocol (only support TCP for now)
   switch((uint8_t)GET_PROTOCOL(pckt))
   {
      case TCP:
         return true;
      default:
         return false;
   }
}

tag* get_flow_tag(struct DNFC* classifier,
                  u_char* pckt,
                  size_t size,
                  size_t header_size,
                  void* protocol_action)
{
   switch(GET_PROTOCOL(pckt))
   {
      case TCP:
         return DNFC_TCP_get_tag(pckt, header_size, protocol_action, classifier->nb_thread);
      default:
         return NULL;
   }
}

/*          Private Functions              */
