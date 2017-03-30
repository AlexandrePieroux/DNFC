/*H**********************************************************************
* FILENAME :        thread_pool.h
*
* DESCRIPTION :
*        A simple implementation of thread pool in POSIX using pthread library.
*
*
* PUBLIC FUNCTIONS :
*           struct threadpool_t   new_threadpool ( uint32_t );
*           int   threadpool_add_queue ( struct threadpool_t, void* (func)(void*), void* );
*           void  threadpool_wait ( struct threadpool_t );
*           void  free_threadpool ( struct threadpool_t );
*
* PUBLIC STRUCTURE :
*       struct threadpool_t
*
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "../memory_management/memory_management.h"
#include "job_queue.h"



struct threadpool_t
{
   pthread_t* threads;
   uint32_t thread_nb;
   struct job_queue* queue;
   bool keep_alive;
   uint32_t threads_alive;
   uint32_t threads_working;
   pthread_mutex_t lock;
   pthread_cond_t idle_cond;
};



struct threadpool_t* new_threadpool(uint32_t nb_threads);



int threadpool_add_work(struct threadpool_t *pool, void* (work)(void*), void* arg);



void threadpool_wait(struct threadpool_t *pool);



void free_threadpool(struct threadpool_t *pool);
