#include "DNFC.h"


/*          Private Functions              */

#define tag uint32_t
#define new_tag chkmalloc(sizeof(tag))

#define IPV4_PROTOCOL                     4
#define MIN_HEADER_LENGTH                 120

#define GET_VERSION(pckt)          (pckt[0] & 240)
#define GET_IHL(pckt)              (pckt[0] & 15)
#define GET_PROTOCOL(pckt)          pckt[9]

#define TCP                               6
#define TCP_FIN(pckt)              (pckt[13] & 1)
#define TCP_ACK(pckt)              (pckt[13] & 16)

#define GET_HEADER_LENGTH(pckt)    (GET_IHL(pckt) * 32)


struct DNFC_pckt* get_DNFC_pckt(u_char* pckt,
                                size_t pckt_length);

bool is_supported(u_char* pckt,
                  size_t pckt_size,
                  size_t header_size);

void init_queue_n_table_pair(struct tuple* out,
                             size_t nb_threads,
                             size_t queue_limit);

tag* get_flow_tag(u_char* pckt,
                  size_t size,
                  size_t header_size,
                  struct hash_table* flow_table);

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
      (*rules)[i]->action = chkcalloc(1, sizeof(struct tuple*));
   
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
   struct tuple* queue_n_table;
   if(!hypercuts_search(classifier->static_classifier, pckt, header_length, (void**)&queue_n_table))
   {
      classifier->callback(pckt, pckt_length);
      return;
   }
   
   if(!queue_n_table->a || !queue_n_table->b)
      init_queue_n_table_pair(queue_n_table, classifier->nb_thread, classifier->queue_limit);
   
   // Get the queue and the dynamic classifier for the matched rule
   struct hash_table* flow_table = queue_n_table->a;
   struct queue* flow_queue = queue_n_table->b;
   
   // Search for a match in the dynamic classifier
   tag* flow_tag = get_flow_tag(pckt, pckt_length, header_length, flow_table);
   
   // We build a pair with the tag and the packet
   struct tuple* packet_result = chkmalloc(sizeof(*packet_result));
   packet_result->a = flow_tag;
   packet_result->b = pckt;
   
   // We push the pair in the queue of the static rule
   queue_push(flow_queue, packet_result);
}



void free_DNFC(struct DNFC* classifier)
{
   return; // TODO
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

void init_queue_n_table_pair(struct tuple* out,
                             size_t nb_threads,
                             size_t queue_limit)
{
   out->a = new_queue(queue_limit, nb_threads);
   out->b = new_hash_table(FNV_1, nb_threads);
}

tag* get_flow_tag(u_char* pckt,
                  size_t size,
                  size_t header_size,
                  struct hash_table* flow_table)
{
   tag* flow_tag;
   switch(GET_PROTOCOL(pckt))
   {
      case TCP:
         flow_tag = get_flow(flow_table, pckt, header_size);
         if(!flow_tag)
         {
            flow_tag = new_tag;
            *flow_tag = TCP_FIN(pckt);
            put_flow(flow_table, pckt, header_size, flow_tag);
         }
         
         if((TCP_FIN(pckt) && TCP_ACK(pckt)) || (*flow_tag && TCP_ACK(pckt)))
            remove_flow(flow_table, pckt, header_size);
         else
            *flow_tag = TCP_FIN(pckt);
            
         return flow_tag;
      default:
         return NULL;
   }
}

/*          Private Functions              */
