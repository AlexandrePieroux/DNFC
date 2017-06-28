#include "DNFC.h"


/*          Private Functions              */

#define tag uint32_t
#define new_tag chkmalloc(sizeof(tag))

#define IPV4_PROTOCOL             4
#define MIN_HEADER_LENGTH         120

#define GET_VERSION(pckt)    pckt[0] & 240
#define GET_IHL(pckt)        pckt[0] & 15
#define GET_PROTOCOL(pckt)   pckt[9]

#define TCP                       6
#define TCP_FIN(pckt)        pckt[13] & 1
#define TCP_ACK(pckt)        pckt[13] & 16



struct tuple
{
   void* a;
   void* b;
};

struct DNFC_pckt
{
   size_t header_lenght;
   uint8_t version;
   uint8_t IHL;
   uint8_t protocol;
};

struct DNFC_pckt* get_DNFC_pckt(u_char* pckt, size_t pckt_length);

bool is_supported(struct DNFC_pckt* pckt);

void init_queue_n_table_pair(struct tuple* out, size_t nb_threads, size_t queue_limit);

tag* get_flow_tag(struct DNFC_pckt* header, u_char* pckt, struct hash_table* flow_table);

/*          Private Functions              */



struct DNFC* new_DNFC(size_t nb_threads,
                      struct classifier_rule ***rules,
                      size_t nb_rules,
                      size_t queue_limit,
                      bool verbose)
{
   // Allocate the structure
   struct DNFC* result = chkmalloc(sizeof(*result));
   for (uint32_t i = 0; i < nb_rules; ++i)
      (*rules)[i]->action = chkmalloc(sizeof(struct tuple*));
   
   // Create the hypercut tree structure for static classification
   result->static_classifier = new_hypercuts_classifier(rules, nb_rules, verbose);
   result->nb_thread = nb_threads;
   
   return result;
}



void DNFC_process(struct DNFC* classifier, u_char* pckt, size_t pckt_length)
{
   // We do a little of packet parsing
   struct DNFC_pckt* DNFC_pckt = get_DNFC_pckt(pckt, pckt_length);
   size_t header_length = DNFC_pckt->header_lenght;
   
   // We check first that the protocol is supported
   if(!is_supported(DNFC_pckt))
      return; // TODO see what we can do with a callback function

   // Search for a match in the static classifier
   struct tuple* queue_n_table;
   
   if(!hypercuts_search(classifier->static_classifier, pckt, header_length, (void**)&queue_n_table))
      return; //Can put to regular stack - for now this should drop the packet instead of returning
   
   if(!queue_n_table->a || !queue_n_table->b)
      init_queue_n_table_pair(queue_n_table, classifier->nb_thread, classifier->queue_limit);
   
   // Get the queue and the dynamic classifier for the matched rule
   struct hash_table* flow_table = queue_n_table->a;
   struct queue* flow_queue = queue_n_table->b;
   
   // Search for a match in the dynamic classifier
   tag* flow_tag = get_flow_tag(DNFC_pckt, pckt, flow_table);
   
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

struct DNFC_pckt* get_DNFC_pckt(u_char* pckt, size_t pckt_length)
{
   uint8_t version = GET_VERSION(pckt);
   
   // We do not support other protocol than IPV4 (e.g. IPV6)
   if(version != IPV4_PROTOCOL)
      return NULL;
   
   // We check the header length
   uint8_t IHL = GET_IHL(pckt);
   uint32_t header_length = IHL * 32;
   if(pckt_length <= header_length || pckt_length < MIN_HEADER_LENGTH)
      return NULL;
   
   // We get the other informations
   struct DNFC_pckt* pckt_result = chkmalloc(sizeof(*pckt_result));
   pckt_result->version = version;
   pckt_result->IHL = IHL;
   pckt_result->protocol = GET_PROTOCOL(pckt);
   
   return pckt_result;
}

bool is_supported(struct DNFC_pckt* pckt)
{
   switch(pckt->protocol)
   {
      case TCP:
         return true;
      default:
         return false;
   }
}

void init_queue_n_table_pair(struct tuple* out, size_t nb_threads, size_t queue_limit)
{
   out->a = new_queue(queue_limit, nb_threads);
   out->b = new_hash_table(FNV_1, nb_threads);
}

tag* get_flow_tag(struct DNFC_pckt* DNFC_pckt, u_char* pckt, struct hash_table* flow_table)
{
   tag* flow_tag;
   switch(DNFC_pckt->protocol)
   {
      case TCP:
         flow_tag = get_flow(flow_table, pckt, DNFC_pckt->header_lenght);
         if(!flow_tag)
         {
            flow_tag = new_tag;
            *flow_tag = TCP_FIN(pckt); // FIN tack in the TCP header
            put_flow(flow_table, pckt, DNFC_pckt->header_lenght, flow_tag);
         }
         
         if((TCP_FIN(pckt) && TCP_ACK(pckt)) || (*flow_tag && TCP_ACK(pckt)))
            remove_flow(flow_table, pckt, DNFC_pckt->header_lenght);
         else
            *flow_tag = TCP_FIN(pckt);
            
         return flow_tag;
      default:
         return NULL;
   }
}

/*          Private Functions              */
