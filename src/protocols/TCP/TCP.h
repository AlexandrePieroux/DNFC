#ifndef _DNFC_TCPH_
#define _DNFC_TCPH_

#include <stdlib.h>
#include "../../../libs/byte_stream/byte_stream.h"

#define TCP 6

typedef unsigned char u_char;

void* DNFC_TCP_get_tag(u_char* pckt,
                       size_t header_size);

void DNFC_TCP_free_tag(void* tag);

#endif
