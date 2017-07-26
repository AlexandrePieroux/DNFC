#ifndef _PCKT_PASINGH_
#define _PCKT_PASINGH_

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>

void parse_ethh(struct ether_header* ethh,
                struct ip** ipv4_out,
                struct ip6_hdr** ipv6_out);

void parse_ipv4h(struct ip** ipv4h,
                 struct tcphdr** tcp_out,
                 struct udphdr** udp_out);

void parse_ipv6h(struct ip6_hdr** ipv6h,
                 struct tcphdr** tcp_out,
                 struct udphdr** udp_out);

#endif
