#include "flow_table.h"

# define HEADER_LENGTH  104

uint32_t get_uint32_value(u_char *header,
                          uint32_t offset,
                          uint32_t length);
key_type get_key(u_char *header);

/*             Flow Table private functions         */



void* get_flow(struct hash_table* table,
               u_char *header,
               size_t header_length)
{
   if(header_length < HEADER_LENGTH)
      return NULL;
   
   // Get the information of the flow
   key_type key = get_key(header);
   void* result = hash_table_get(table, key);
   //free_byte_stream(key);
   return result;
}



bool put_flow(struct hash_table* table,
              u_char *header,
              size_t header_length,
              void* tag)
{
   if(header_length < HEADER_LENGTH)
      return false;
   
   // Get the information of the flow
   key_type key = get_key(header);
   if (!hash_table_put(table, key, tag))
   {
      free_byte_stream(key);
      return false;
   }
   return true;
}



bool remove_flow(struct hash_table* table,
                 u_char *header,
                 size_t header_length)
{
   if(header_length < HEADER_LENGTH)
      return false;
   
   // Get the information of the flow
   key_type key = get_key(header);
   bool result = hash_table_remove(table, key);
   free_byte_stream(key);
   return result;
}



/*             Flow Table private functions         */

uint32_t get_uint32_value(u_char *header,
                          uint32_t offset,
                          uint32_t length)
{
   uint32_t char_index_start = offset / 8;
   uint32_t char_index_end = (offset + length - 1) / 8;
   
   // Getting the first part of the value and cleaning leading bits
   uint32_t result = header[char_index_start];
   uint32_t cleaning_mask = (uint32_t)0x1 << (8 - (offset % 8));
   cleaning_mask--;
   result = result & cleaning_mask;
   char_index_start++;
   
   // Getting the rest of the value
   for (; char_index_start <= char_index_end; ++char_index_start)
   {
      result = result << 8;
      result = result | header[char_index_start];
   }
   
   // Cleaning the end
   uint32_t rest = (offset + length) % 8;
   if (rest != 0)
      result = result >> (8 - rest);
   return result;
}


key_type get_key(u_char *header)
{
   key_type key = new_byte_stream();
   uint32_t value = get_uint32_value(header, PROTOCOL * 8, 8);
   append_bytes(key, &value, 1);
   value = get_uint32_value(header, S_ADDR * 8, 32);
   append_bytes(key, &value, 4);
   value = get_uint32_value(header, D_ADDR * 8, 32);
   append_bytes(key, &value, 4);
   value = get_uint32_value(header, S_PORT * 8, 16);
   append_bytes(key, &value, 2);
   value = get_uint32_value(header, D_PORT * 8, 16);
   append_bytes(key, &value, 2);
   return key;
}
