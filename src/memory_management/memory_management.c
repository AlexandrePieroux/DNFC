#include "memory_management.h"

// Macro that ease the checking of a pointer and error printing.
#define MEMORYCHECK(function, result) if(!result) \
                                                         { \
                                                            fprintf(stderr, "%s ERROR: System out of memory\n", function); \
                                                            exit(EXIT_FAILURE); \
                                                         } 


void* chkmalloc(size_t size)
{
   void* result = malloc(size);
   MEMORYCHECK("MALLOC", result)
   return result;
}



void* chkcalloc(size_t nitems, size_t size)
{
   void* result = calloc(nitems, size);
   MEMORYCHECK("CALLOC", result)
   return result;
}



void* chkrealloc(void* ptr, size_t size)
{
   void* result = realloc(ptr, size);
   MEMORYCHECK("REALLOC", result)
   return result;
}
