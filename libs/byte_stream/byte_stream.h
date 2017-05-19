#ifndef _BYTE_STREAMH_
#define _BYTE_STREAMH_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "../memory_management/memory_management.h"


struct byte_stream
{
   uint8_t* stream;
   size_t size;
};

struct byte_stream* new_byte_stream(void);

void free_byte_stream(struct byte_stream* bt);

void append_bytes(struct byte_stream* bt, void* data, size_t size);

bool byte_stream_gt(struct byte_stream* bt1, struct byte_stream* bt2);

bool byte_stream_eq(struct byte_stream* bt1, struct byte_stream* bt2);

#endif
