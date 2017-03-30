#define _XOPEN_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>

#define FNV_prime            16777619
#define FNV_offset         2166136261



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
uint32_t FNV1_hash(uint32_t value);
