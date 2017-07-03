#include "TCP.h"

/*  Private function  */

key_type get_key(u_char *header, size_t header_size);



/*  Public function  */

void* DNFC_TCP_new_tag(u_char* pckt,
                       size_t header_size)
{
   key_type key = get_key(pckt, header_size);
   return key;
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
