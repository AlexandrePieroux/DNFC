#ifndef _MEMORY_MANAGEMENTH_
#define _MEMORY_MANAGEMENTH_

#include <stdio.h>
#include <stdlib.h>

void* chkmalloc(size_t size);

void* chkcalloc(size_t nitems, size_t size);

void* chkrealloc(void* ptr, size_t size);

#endif
