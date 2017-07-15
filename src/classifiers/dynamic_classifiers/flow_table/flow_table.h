#ifndef _FLOW_TABLEH_
#define _FLOW_TABLEH_

/*H**********************************************************************
 * FILENAME :        flow_table.h
 *
 * DESCRIPTION :
 *
 * PUBLIC STRUCTURE :
 *
 *
 * AUTHOR :    Pieroux Alexandre
 *H*/

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>

#include "../../classifier_rule/classifier_field.h"
#include "../../../../libs/hash_table/hash_table.h"

typedef unsigned char u_char;



void* get_flow(struct hash_table* table, u_char* pckt);

bool put_flow(struct hash_table* table, u_char* pckt, void* tag);

bool remove_flow(struct hash_table* table, u_char* pckt);

#endif
