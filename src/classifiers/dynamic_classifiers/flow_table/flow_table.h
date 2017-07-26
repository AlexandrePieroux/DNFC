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

#include "../../classifier_rule/classifier_field.h"
#include "../../../../libs/packet_parsing/packet_parsing.h"
#include "../../../../libs/hash_table/hash_table.h"

typedef unsigned char u_char;
typedef struct hash_table flow_table;

void* get_flow(flow_table* table, u_char* pckt);

key_type put_flow(flow_table* table, u_char* pckt, void* tag);

bool remove_flow(flow_table* table, u_char* pckt);

#endif
