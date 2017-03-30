/*H**********************************************************************
* FILENAME :        memory_management.h
*
* DESCRIPTION :
*        Memory utility function that ease memory allocation and check. It also
*        centralize the memory functions.
*
*
* PUBLIC FUNCTIONS :
*        void* chkmalloc ( size_t )             
*        void* chkcalloc ( size_t , size_t )
*
*
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdio.h>
#include <stdlib.h>



/*******************************************************************
* NAME :                 void* chkmalloc ( size_t )
*
* DESCRIPTION :      Encapsulation of the malloc function that does not allow
*                                        the return of NULL value. In case of memory allocation failure
*                                        the program exit.
*
* INPUTS :
*       PARAMETERS:
*           size_t size                The size of the memory to allocate.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            Type:     void* 
*            Values:  memory             Sucessfully allocated memory.
*
*
* NOTES :      NONE
*******************************************************************/
void* chkmalloc(size_t size);



/*******************************************************************
* NAME :                 void* chkcalloc ( size_t , size_t );
*
* DESCRIPTION :      Encapsulation of the calloc function that does not allow
*                                        the return of NULL value. In case of memory allocation failure
*                                        the program exit.
*
* INPUTS :
*       PARAMETERS:
*           size_t size                The size of the memory to allocate.
*           size_t nitems           The number of item to allocate.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            Type:     void* 
*            Values:  memory             Sucessfully allocated memory.
*
*
* NOTES :      NONE
*******************************************************************/
void* chkcalloc(size_t nitems, size_t size);



/*******************************************************************
* NAME :                 void* chkrealloc ( size_t , size_t );
*
* DESCRIPTION :      Encapsulation of the realloc function that does not allow
*                                        the return of NULL value. In case of memory allocation failure
*                                        the program exit.
*
* INPUTS :
*       PARAMETERS:
*           void* ptr                The previous pointer to the memory.
*           size_t size           The new size.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            Type:     void* 
*            Values:  memory             Sucessfully allocated memory.
*
*
* NOTES :      NONE
*******************************************************************/
void* chkrealloc(void* ptr, size_t size);
