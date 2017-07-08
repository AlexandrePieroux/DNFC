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
#include "../../../../libs/hash_table/hash_table.h"

typedef unsigned char u_char;



void* get_flow(struct hash_table* table, u_char *header, size_t header_length);

bool put_flow(struct hash_table* table, u_char *header, size_t header_length, void* tag);

bool remove_flow(struct hash_table* table, u_char *header, size_t header_length);

#endif
