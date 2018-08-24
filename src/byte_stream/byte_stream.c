#include "byte_stream.h"



struct byte_stream* new_byte_stream()
{
    struct byte_stream* result = chkmalloc(sizeof(struct byte_stream*));
    result->stream = chkmalloc(sizeof(result->stream));
    result->size = 0;
    return result;
}



void free_byte_stream(struct byte_stream* bt)
{
    if(!bt)
       return;
   
    if(bt->stream)
    {
        free(bt->stream);
        bt->stream = NULL;
    }
    free(bt);
    bt = NULL;
}



void append_bytes(struct byte_stream* bt, void* data, size_t size)
{
    uint8_t* c = (uint8_t*)data;
    bt->stream = chkrealloc(bt->stream, bt->size + size);
    uint32_t j = 0;
    for(uint32_t i = bt->size + size; i > bt->size; i--)
    {
        bt->stream[i - 1] = c[j];
        j++;
    }
    bt->size += size;
}



bool byte_stream_gt(struct byte_stream* bt1, struct byte_stream* bt2)
{
    if(bt1->size > bt2->size)
        return true;
    else
        return false;
    
    for (uint32_t i = 0; i < bt1->size; ++i)
    {
        if(bt1->stream[i] > bt2->stream[i])
            return true;
        else if (bt1->stream[i] < bt2->stream[i])
            return false;
    }
    return false;
}



bool byte_stream_eq(struct byte_stream* bt1, struct byte_stream* bt2)
{
    if(!bt1 || !bt2 || !(bt1->stream) || !(bt2->stream))
       return false;
   
    if(bt1->size != bt2->size)
        return false;
    
    for (uint32_t i = 0; i < bt1->size; ++i)
    {
        if(bt1->stream[i] != bt2->stream[i])
            return false;
    }
    return true;
}