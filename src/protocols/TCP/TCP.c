#include "TCP.h"

typedef struct byte_stream* key_type;
#define TCP_HEADER_LENGTH     160
#define TCP_DATA_OFFSET(pckt, hdr_s) pckt[hdr_s + 12] & 240

/*  Private function  */

key_type get_key(u_char *header, size_t header_size);



/*  Public function  */

void* DNFC_TCP_get_tag(u_char* pckt,
                       size_t header_size,
                       void* protocol_action,
                       size_t nb_threads)
{
   // Check that the packet is long enought
   if((TCP_DATA_OFFSET(pckt, header_size/8)) < TCP_HEADER_LENGTH)
      return NULL;
   
   // Retrieve the flow table of the rule that matched
   if(!protocol_action)
      protocol_action = new_hash_table(FNV_1, nb_threads);
   
   // Prepare to insert the packet in the linked list of packets of the flow
   key_type new_flow_key = get_key(pckt, header_size/8);
   struct linked_list* pckt_llist = new_linked_list(new_flow_key, FNV_1, pckt);
   
   // Retrieve the list of packets for that flow
   struct tuple* tag = (struct tuple*) get_flow(protocol_action, pckt, header_size);
   if(!tag)
   {
      // Here, the TCP's tag is a tuple that contain the linked list representing the packets
      // received for the matched flow and the hazardous pointers for that list. Note: HPs are
      // mandatory to manipulate the linked list in a concurrent environement
      tag = chkmalloc(sizeof(*tag));
      tag->a = pckt_llist;
      tag->b = linked_list_init(nb_threads);
      
      put_flow(protocol_action, pckt, header_size, tag);
   } else {
      linked_list_insert(tag->b, (struct linked_list**)&tag->a, pckt_llist);
   }
   
   return tag;
}



void DNFC_TCP_free_tag(void* tag)
{
   key_type key = (key_type)tag;
   free_byte_stream(key);
}



/*  Private function  */

key_type get_key(u_char *header, size_t header_size)
{
   key_type key = new_byte_stream();
   
   // Protocol
   append_bytes(key, &header[9], 1);
   
   // Source address and destination address
   append_bytes(key, &header[12], 4);
   append_bytes(key, &header[16], 4);
   
   // Source port and destination port
   append_bytes(key, &header[header_size], 1);
   append_bytes(key, &header[header_size + 1], 1);
   return key;
}
