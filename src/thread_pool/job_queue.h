/*H**********************************************************************
* FILENAME :        job_queue.h
*
* DESCRIPTION :
*        Implementation of blocking job queue .
*
*
* PUBLIC FUNCTIONS :
*
* PUBLIC STRUCTURE :
*       struct linked_list
*
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "../memory_management/memory_management.h"



struct job_queue
{
   struct job* head;
   struct job* tail;
   bool has_jobs;
   pthread_mutex_t lock;
   pthread_mutex_t mutex;
   pthread_cond_t has_jobs_cond;
};



struct job
{
   struct job* next;
   void* (*work)(void* arg);
   void* arg;
};



struct job_queue* new_job_queue(void);



int job_queue_push(struct job_queue* queue, struct job* job);



struct job* job_queue_pull(struct job_queue* queue);



void job_queue_clear(struct job_queue* queue);



void job_queue_wait(struct job_queue* queue);



void job_queue_free(struct job_queue* queue);
