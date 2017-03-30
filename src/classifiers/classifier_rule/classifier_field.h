/*H**********************************************************************
* FILENAME :        classifier_field.h
*
* DESCRIPTION :
*        A hash table implementation that use integer hashing and dynamic
*        incremental resiszing. The colisions are handled with linked_list structures.
*
* PUBLIC STRUCTURE :
*       struct classifier_field
*
* 
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdint.h>

#define PROTOCOL 9
#define S_ADDR 12
#define D_ADDR 16
#define S_PORT 20
#define D_PORT 22

struct classifier_field
{
   uint32_t id;
   uint32_t bit_length;
   uint32_t offset;
   uint32_t mask;
   uint32_t value;
};
