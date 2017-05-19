#ifndef _MURMUR3H_
#define _MURMUR3H_

#define _XOPEN_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include "../byte_stream/byte_stream.h"

typedef struct byte_stream* key_type;
#define MURMUR_seed  2166136261



/*******************************************************************
* NAME :                 uint32_t hash(void* parameters, uint32_t value)
*
* DESCRIPTION :      Hash a given value using the given parameters.
*
* INPUTS :
*       PARAMETERS:
*           void* parameters             The parameters of the hash function.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*            None
*       RETURN :
*            Type:     uint32_t 
*            Values:   The value hashed.
*
* NOTES :  
*******************************************************************/
uint32_t murmur_hash(key_type value);

#endif
