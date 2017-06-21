#include "DNFC.h"


/*          Private Functions              */

struct pair* new_queue_n_table_pair(size_t nb_threads, size_t queue_limit);

struct tag* new_tag(u_char* header header, size_t header_length);

bool is_last_packet_of_flow(u_char* header, size_t header_length);

/*          Private Functions              */



struct DNFC* new_DNFC(size_t nb_threads, struct classifier_rule ***rules, size_t nb_rules, size_t queue_limit, bool verbose)
{
   // Allocate the structure and create a queue and a flow table for each rules
   struct DNFC* result = chkmalloc(sizeof(*result));
   for (uint32_t i = 0; i < nb_rules; ++i)
      (*rules)[i]->action = new_queue_n_table_pair(nb_threads, queue_limit);
   
   // Create the hypercut tree structure for static classification
   result->static_classifier = new_hypercuts_classifier(rules, nb_rules, verbose);
   return result;
}



void DNFC_process(struct DNFC* classifier, u_char* header, size_t header_length)
{
   // Search for a match in the static classifier
   struct pair* queue_n_table = (struct pair*) hypercuts_search(classifier->static_classifier, header, header_length);
   
   // TODO
   if(!queue_n_table)
      return; //Can put to regular stack - for now this should drop the packet instead of returning
   
   // Get the queue and the dynamic classifier for the matched rule
   struct hash_table* flow_table = queue_n_table->a;
   struct queue* flow_queue = queue_n_table->b;
   
   // Search for a match in the dynamic classifier
   struct tag* tag = (struct tag*) get_flow(flow_table, header, header_length);
   
   // If no match, we create a fresh, unique tag
   if(!tag)
   {
      tag = new_tag(header, header_length);
      put_flow(flow_table, header, header_length, tag);
   }
   
   // We check that if the packet is the last of the flow - if yes, we remove it
   if(is_last_packet_of_flow(header, header_length))
      remove_flow(flow_table, header, header_length);
   
   // TODO
   // We build a pair with the tag and the packet header
   struct pair* packet_result = chkmalloc(sizeof(*packet_result));
   pair->a = tag;
   pair->b = header;// -> maybe put the packet inside too
   
   // We push the pair in the queue of the static rule
   queue_push(flow_queue, packet_result);
}



void free_DNFC(struct DNFC* classifier)
{
   return; // TODO
}



/*          Private Functions              */

struct pair* new_queue_n_table_pair(size_t nb_threads, size_t queue_limit)
{
   return NULL; // TODO
}


struct tag* new_tag(u_char* header header, size_t header_length)
{
   return NULL; // TODO
}


bool is_last_packet_of_flow(u_char* header, size_t header_length)
{
   return false; // TODO
}

/*          Private Functions              */
