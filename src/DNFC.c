#include "DNFC.h"


/*          Private Functions              */

struct DNFC_pckt* get_DNFC_pckt(u_char* pckt,
                                size_t pckt_length);

void init_queue(struct queue* queue,
                size_t nb_threads,
                size_t queue_limit);

struct DNFC_tag* get_flow_tag(struct DNFC* classifier,
                              u_char* pckt,
                              size_t pckt_len,
                              flow_table* protocol_action);

key_type get_key(u_char* pckt,
                 size_t pckt_len);

/*          Private Functions              */



struct DNFC* new_DNFC(size_t nb_threads,
                      struct classifier_rule ***rules,
                      uint32_t nb_rules,
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



bool DNFC_process(struct DNFC* classifier,
                  u_char* pckt,
                  size_t pckt_len)
{
   // Search for a match in the static classifier
   struct DNFC_action* action = NULL;
   if(!hypercuts_search(classifier->static_classifier, pckt, pckt_len, (void**)&action))
   {
      if(classifier->callback)
         classifier->callback(pckt, pckt_len);
      return false;
   }
   
   // If no action is found we instantiate a new queue and a new flow table
   if(!action)
   {
      action = chkmalloc(sizeof(struct DNFC_action*));
      action->pckt_queue = new_queue(classifier->queue_limit, classifier->nb_thread);
      action->flow_table = new_hash_table(FNV_1, classifier->nb_thread);
   }
   
   // Search for a match in the dynamic classifier
   struct DNFC_tag* flow_tag = get_flow_tag(classifier, pckt, pckt_len, action->flow_table);
   
   // We build a pair with the tag and the packet
   struct DNFC_tagged_pckt* packet_result = chkmalloc(sizeof(*packet_result));
   packet_result->tag = flow_tag;
   
   packet_result->pckt = chkmalloc(sizeof(struct DNFC_pckt*));
   packet_result->pckt->data = pckt;
   packet_result->pckt->size = pckt_len;
   
   // We push the result in the queue of the static rule
   queue_push(action->pckt_queue, packet_result);
   return true;
}

void DNFC_free_tag(void* tag_item)
{
   key_type key = (key_type)tag_item;
   free_byte_stream(key);
}



/*          Private Functions              */

struct DNFC_tag* get_flow_tag(struct DNFC* classifier,
                  u_char* pckt,
                  size_t pckt_len,
                  flow_table* flow_table)
{
   // Prepare to insert the packet in the linked list of packets of the flow
   key_type new_flow_key = get_key(pckt, pckt_len);
   struct linked_list* pckt_llist = new_linked_list(new_flow_key, FNV1a_hash(new_flow_key), pckt);
   
   // Retrieve the list of packets for that flow
   struct DNFC_tag* flow_tag = get_flow(flow_table, pckt);
   if(!flow_tag)
   {
      // The tag is a tuple that contain the linked list representing the packets
      // received for the matched flow and the hazardous pointers for that list.
      // Note: HPs are mandatory to manipulate the linked list in a concurrent environement
      flow_tag = chkmalloc(sizeof(*flow_tag));
      flow_tag->flow_pckt_list = pckt_llist;
      flow_tag->flow_pckt_list_hps = linked_list_init(classifier->nb_thread);
   
      if(!put_flow(flow_table, pckt, flow_tag)) // Check if the tag was already inserted while building it
      {
         // If it was already inserted we free the one we created
         linked_list_free(&flow_tag->flow_pckt_list);
         free_hp(flow_tag->flow_pckt_list_hps);
         free(flow_tag);
         
         // We are sure here that the tag will be retrieved (as we don't delete it from the flow table yet)
         flow_tag = get_flow(flow_table, pckt);
         linked_list_insert(flow_tag->flow_pckt_list_hps, &flow_tag->flow_pckt_list, pckt_llist);
      }
   } else {
      linked_list_insert(flow_tag->flow_pckt_list_hps, &flow_tag->flow_pckt_list, pckt_llist);
   }
   return flow_tag;
}

/*          Private Functions              */

/*          Parse packet function          */

key_type get_key(u_char* pckt, size_t pckt_len)
{
   key_type key = new_byte_stream();
   
   // Layer 2 protocol
   struct ether_header* ethh = (struct ether_header*)pckt;
   
   // Layer 3 protocols
   struct ip* ip4h = NULL;
   struct ip6_hdr* ip6h = NULL;
   
   // Layer 4 protocols
   struct tcphdr* tcph = NULL;
   struct udphdr* udph = NULL;
   
   // Parse layer 3
   parse_ethh(ethh, &ip4h, &ip6h);
   if(!ip4h && !ip6h)
   {
      fprintf(stderr, "Cannot parse layer 3 protocol!\n");
      return NULL;
   }
   
   // Parse layer 4
   if(ip4h)
      parse_ipv4h(&ip4h, &tcph, &udph);

   if(ip6h)
      parse_ipv6h(&ip6h, &tcph, &udph);
   
   if(!tcph && !udph)
   {
      fprintf(stderr, "Cannot parse layer 4 protocol!\n");
      return NULL;
   }
   
   // Get the information to construct a key for the flow
   if(tcph)
      append_bytes(key, tcph->th_seq, 4);
   else
      append_bytes(key, udph, pckt_len);
   return key;
}
