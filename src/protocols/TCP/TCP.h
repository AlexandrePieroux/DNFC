#ifndef _DNFC_TCPH_
#define _DNFC_TCPH_

#include <time.h>
#include "../../classifiers/dynamic_classifiers/dynamic_classifiers.h"

#define TCP 6

typedef unsigned char u_char;

void* DNFC_TCP_get_tag(u_char* pckt,
                       size_t header_size);

void DNFC_TCP_free_tag(void* tag);

#endif
