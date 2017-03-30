#include "thread_pool.h"



// Atomicaly fetch and increase the value of a given number
#define fetch_and_inc(n) __sync_fetch_and_add (n, 1)

// Atomicaly fetch and decrease the value of a given number
#define fetch_and_dec(n) __sync_fetch_and_sub (n, 1)



/*                                Private function                           */

void thread_work(struct threadpool_t* pool);

void notify_all(struct job_queue* queue);

/*                                Private function                           */



struct threadpool_t* new_threadpool(uint32_t nb_threads)
{
   if(nb_threads == 0)
      return NULL;

   struct threadpool_t* pool = chkmalloc(sizeof *pool);
   if(!(pool->queue = new_job_queue()))
      return NULL;

   pool->threads = chkmalloc(sizeof(pthread_t *) * nb_threads);
   pool->keep_alive = true;
   pool->threads_alive = 0;
   pool->threads_working = 0;
   pool->thread_nb = nb_threads;
   pthread_mutex_init(&(pool->lock), NULL);
   pthread_cond_init(&(pool->idle_cond), NULL);


   for (uint32_t i = 0; i < nb_threads; ++i)
   {
      pthread_create(&(pool->threads[i]), NULL, (void*)thread_work, pool);
      pthread_detach(pool->threads[i]);
   }
   return pool;
}



int threadpool_add_work(struct threadpool_t* pool, void* (work)(void*), void* arg)
{
   struct job* job = chkmalloc(sizeof *job);
   job->next = NULL;
   job->work = work;
   job->arg = arg;

   int result = job_queue_push(pool->queue, job);
   return result;
}



void threadpool_wait(struct threadpool_t *pool)
{
   pthread_mutex_lock(&pool->lock);
   while(pool->queue->head || pool->threads_working)
      pthread_cond_wait(&pool->idle_cond, &pool->lock);
   pthread_mutex_unlock(&pool->lock);
}



void free_threadpool(struct threadpool_t* pool)
{
   if(!pool)
      return;

   pool->keep_alive = false;
   while(pool->threads_alive)
   {
      notify_all(pool->queue);
      sleep(1);
   }

   job_queue_free(pool->queue);
   free(pool->threads);
   free(pool);
}



/*                   Private functions                    */

void thread_work(struct threadpool_t* pool)
{

   fetch_and_inc(&pool->threads_alive);

   while(pool->keep_alive)
   {
      job_queue_wait(pool->queue);
      if(pool->keep_alive)
      {
         struct job* job = job_queue_pull(pool->queue);
         void* (*work)(void* arg);
         void* arg;
         if(job)
         {

            fetch_and_inc(&pool->threads_working);

            work = job->work;
            arg = job->arg;
            work(arg);
            free(job);

            fetch_and_dec(&pool->threads_working);

            pthread_mutex_lock(&pool->lock);
            if(!pool->threads_working)
               pthread_cond_signal(&(pool->idle_cond));
            pthread_mutex_unlock(&pool->lock);
         }
      }
   }

   fetch_and_dec(&pool->threads_alive);
}



void notify_all(struct job_queue* queue)
{
   pthread_mutex_lock(&queue->mutex);
   queue->has_jobs = true;
   pthread_cond_broadcast(&queue->has_jobs_cond);
   pthread_mutex_unlock(&queue->mutex);
}

/*                   Private functions                    */
