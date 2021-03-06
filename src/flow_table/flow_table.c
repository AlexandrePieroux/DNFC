#include "flow_table.h"

key_type get_key(u_char *pckt);

/*             Flow Table private functions         */



void* get_flow(flow_table* table,
               u_char* pckt)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   if(!key)
      return NULL;
   void* result = hash_table_get(table, key);
   free_byte_stream(key);
   return result;
}



key_type put_flow(flow_table* table,
                  u_char* pckt,
                  void* tag)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   if(!key)
      return NULL;
   if (!hash_table_put(table, key, tag))
   {
      free_byte_stream(key);
      return NULL;
   }
   return key;
}



bool remove_flow(flow_table* table,
                 u_char* pckt)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   if(!key)
      return NULL;
   bool result = hash_table_remove(table, key);
   free_byte_stream(key);
   return result;
}



/*             Flow Table private functions         */

key_type get_key(u_char* pckt)
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
      free_byte_stream(key);
      return NULL;
   }
   append_bytes(key, &ethh->ether_type, 1);
   
   // Parse layer 4
   if(ip4h)
   {
      parse_ipv4h(&ip4h, &tcph, &udph);
      append_bytes(key, &ip4h->ip_src, 4);
      append_bytes(key, &ip4h->ip_dst, 4);
   }
   if(ip6h)
   {
      parse_ipv6h(&ip6h, &tcph, &udph);
      append_bytes(key, &ip6h->ip6_src, 16);
      append_bytes(key, &ip6h->ip6_dst, 16);
   }
   if(!tcph && !udph)
   {
      fprintf(stderr, "Cannot parse layer 4 protocol!\n");
      return NULL;
   }
   
   // Get the information to construct a key for the flow
   if(tcph)
   {
      append_bytes(key, &tcph->th_sport, 2);
      append_bytes(key, &tcph->th_dport, 2);
   } else if(udph) {
      append_bytes(key, &udph->uh_sport, 2);
      append_bytes(key, &udph->uh_dport, 2);
   }
   
   return key;
}
