#include "packet_parsing.h"

void parse_ethh(struct ether_header* ethh,
                struct ip** ipv4_out,
                struct ip6_hdr** ipv6_out)
{
   switch (ntohs(ethh->ether_type))
   {
      case ETHERTYPE_IP:
         *ipv4_out = ((struct ip *)(ethh + 1));
         return;
      case ETHERTYPE_IPV6:
         *ipv6_out = ((struct ip6_hdr *)(ethh + 1));
         return;
      default:
         return;
   }
}



void parse_ipv4h(struct ip** ipv4h,
                 struct tcphdr** tcp_out,
                 struct udphdr** udp_out)
{
   switch ((*ipv4h)->ip_p)
   {
      case IPPROTO_TCP:
         *tcp_out = (struct tcphdr *)((uint8_t *)(*ipv4h) + ((*ipv4h)->ip_hl<<2));
         return;
      case IPPROTO_UDP:
         *udp_out = (struct udphdr *)((uint8_t *)(*ipv4h) + ((*ipv4h)->ip_hl<<2));
         return;
      case IPPROTO_IPV4:
         *ipv4h = ((struct ip *)((uint8_t *)(*ipv4h) + ((*ipv4h)->ip_hl<<2)));
         parse_ipv4h(ipv4h, tcp_out, udp_out);
         return;
      default:
         return;
   }
}



void parse_ipv6h(struct ip6_hdr** ipv6h,
                 struct tcphdr** tcp_out,
                 struct udphdr** udp_out)
{
   switch (ntohs((*ipv6h)->ip6_ctlun.ip6_un1.ip6_un1_nxt))
   {
      case IPPROTO_TCP:
         *tcp_out = (struct tcphdr *)((*ipv6h) + 1);
         return;
      case IPPROTO_UDP:
         *udp_out = (struct udphdr *)((*ipv6h) + 1);
         return;
      case IPPROTO_IPV4:
         *ipv6h = (*ipv6h) + 1;
         parse_ipv4h((struct ip **)ipv6h, tcp_out, udp_out);
         return;
      case IPPROTO_IPV6:
         *ipv6h = (struct ip6_hdr *)((*ipv6h) + 1);
         parse_ipv6h(ipv6h, tcp_out, udp_out);
         return;
   }
}
