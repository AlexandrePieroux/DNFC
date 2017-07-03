#include "TCP.h"

typedef struct byte_stream* key_type;
#define TCP_HEADER_LENGTH     160

#define TCP_DATA_OFFSET(pckt, hdr_s) pckt[hdr_s + 12] & 240

/*  Private function  */

key_type get_key(u_char *header, size_t header_size);



/*  Public function  */

void* DNFC_TCP_get_tag(u_char* pckt,
                       size_t header_size)
{
   if((TCP_DATA_OFFSET(pckt, header_size/8)) < TCP_HEADER_LENGTH)
      return NULL;
   
   key_type key = get_key(pckt, header_size/8);
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
