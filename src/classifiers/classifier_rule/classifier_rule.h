#ifndef _CLASSIFIER_RULEDH_
#define _CLASSIFIER_RULEDH_

/*H**********************************************************************
* FILENAME :        classifier_rule.h
*
* DESCRIPTION :
*        A hash table implementation that use integer hashing and dynamic
*        incremental resiszing. The colisions are handled with linked_list structures.
*
*
* PUBLIC STRUCTURE :
*       struct classifier_rule
*
* 
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdint.h>
#include "classifier_field.h"

struct classifier_rule
{
   uint32_t id;
   struct classifier_field **fields;
   uint32_t nb_fields;
   void *action;
};

#endif
