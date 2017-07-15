#include "flow_table.h"

key_type get_key(u_char *pckt);

void parse_ethh(struct ether_header* ethh,
                struct ip* ipv4_out,
                struct ip6_hdr* ipv6_out);

void parse_ipv4h(struct ip* ipv4h,
                 struct tcphdr* tcp_out,
                 struct udphdr* udp_out);

void parse_ipv6h(struct ip6_hdr* ipv6h,
                 struct tcphdr* tcp_out,
                 struct udphdr* udp_out);

/*             Flow Table private functions         */



void* get_flow(struct hash_table* table,
               u_char* pckt)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   void* result = hash_table_get(table, key);
   free_byte_stream(key);
   return result;
}



bool put_flow(struct hash_table* table,
              u_char* pckt,
              void* tag)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   if (!hash_table_put(table, key, tag))
   {
      free_byte_stream(key);
      return false;
   }
   return true;
}



bool remove_flow(struct hash_table* table,
                 u_char* pckt)
{
   // Get the information of the flow
   key_type key = get_key(pckt);
   bool result = hash_table_remove(table, key);
   free_byte_stream(key);
   return result;
}



/*             Flow Table private functions         */

key_type get_key(u_char* pckt)
{
   key_type key = new_byte_stream();
   struct ether_header* ethh = (struct ether_header*)pckt;
   
   // Layer 3 protocols
   struct ip* ip4h = NULL;
   struct ip6_hdr* ip6h = NULL;
   
   // Layer 4 protocols
   struct tcphdr* tcph = NULL;
   struct udphdr* udph = NULL;
   
   // Parse layer 3
   parse_ethh(ethh, ip4h, ip6h);
   if(!ip4h && !ip6h)
   {
      fprintf(stderr, "Cannot parse layer 3 protocol!\n");
      free_byte_stream(key);
      return NULL;
   }
   append_bytes(key, ethh->ether_type, 1);
   
   // Parse layer 4
   if(ip4h)
   {
      parse_ipv4h(ip4h, tcph, udph);
      append_bytes(key, &ip4h->ip_src, 4);
      append_bytes(key, &ip4h->ip_dst, 4);
   }
   if(ip6h)
   {
      parse_ipv6h(ip6h, tcph, udph);
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
      append_bytes(key, tcph->th_sport, 2);
      append_bytes(key, tcph->th_dport, 2);
   } else if(udph) {
      append_bytes(key, udph->uh_sport, 2);
      append_bytes(key, udph->uh_dport, 2);
   }
   
   return key;
}



void parse_ethh(struct ether_header* ethh,
                struct ip* ipv4_out,
                struct ip6_hdr* ipv6_out)
{
   switch (ntohs(ethh->ether_type))
   {
      case ETHERTYPE_IP:
         ipv4_out = ((struct ip *)(ethh + 1));
         return;
      case ETHERTYPE_IPV6:
         ipv6_out = ((struct ip6_hdr *)(ethh + 1));
         return;
      default:
         return;
   }
}



void parse_ipv4h(struct ip* ipv4h,
                 struct tcphdr* tcp_out,
                 struct udphdr* udp_out)
{
   switch (ipv4h->ip_p)
   {
      case IPPROTO_TCP:
         tcp_out = (struct tcphdr *)((uint8_t *)ipv4h + (ipv4h->ip_hl<<2));
         return;
      case IPPROTO_UDP:
         udp_out = (struct udphdr *)((uint8_t *)ipv4h + (ipv4h->ip_hl<<2));
         return;
      case IPPROTO_IPV4:
         ipv4h = ((struct ip *)((uint8_t *)ipv4h + (ipv4h->ip_hl<<2)));
         return parse_ipv4h(ipv4h, tcp_out, udp_out);
      default:
         return;
   }
}



void parse_ipv6h(struct ip6_hdr* ipv6h,
                 struct tcphdr* tcp_out,
                 struct udphdr* udp_out)
{
   switch (ntohs(ipv6h->ip6_ctlun.ip6_un1.ip6_un1_nxt))
   {
      case IPPROTO_TCP:
         tcp_out = (struct tcphdr *)(ipv6h + 1);
         return;
      case IPPROTO_UDP:
         udp_out = (struct udphdr *)(ipv6h + 1);
         return;
      case IPPROTO_IPV4:
         return parse_ipv4h((struct ip *)(ipv6h + 1), tcp_out, udp_out);
      case IPPROTO_IPV6:
         ipv6h = (struct ip6_hdr *)(ipv6h + 1);
         return parse_ipv6h(ipv6h, tcp_out, udp_out);
      }
}
