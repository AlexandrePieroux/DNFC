#include "job_queue.h"



/*                                   Private function                                 */

void notify_jobs(struct job_queue* queue);

/*                                   Private function                                 */



struct job_queue* new_job_queue(void)
{
   struct job_queue* queue = chkmalloc(sizeof *queue);
   queue->head = NULL;
   queue->tail = NULL;
   queue->has_jobs = false;
   pthread_mutex_init(&(queue->lock), NULL);
   pthread_mutex_init(&(queue->mutex), NULL);
   pthread_cond_init(&(queue->has_jobs_cond), NULL);
   return queue;
}



int job_queue_push(struct job_queue* queue, struct job* job)
{
   if(!job || !queue)
      return -1;

   job->next = NULL;

   pthread_mutex_lock(&(queue->lock));
   if(queue->head)
   {
      queue->tail->next = job;
   } else {
      queue->head = job;
   }
   queue->tail = job;
   notify_jobs(queue);
   pthread_mutex_unlock(&(queue->lock));

   return 1;
}



struct job* job_queue_pull(struct job_queue* queue)
{
   if(!queue)
      return NULL;

   struct job* job = NULL;
   pthread_mutex_lock(&(queue->lock));
   if(queue->head)
   {
      job = queue->head;
      queue->head = job->next;
      job->next = NULL;

      if(queue->head)
         notify_jobs(queue);
   }
   pthread_mutex_unlock(&(queue->lock));
   return job;
}



void job_queue_clear(struct job_queue* queue)
{
   if(!queue)
      return;

   pthread_mutex_lock(&(queue->lock));
   while(queue->head)
   {
      struct job* job = queue->head;
      queue->head = job->next;
      free(job);
   }

   queue->head = NULL;
   queue->tail = NULL;
   pthread_mutex_lock(&queue->mutex);
   queue->has_jobs = false;
   pthread_mutex_unlock(&queue->mutex);

   pthread_mutex_unlock(&(queue->lock));
}



void job_queue_wait(struct job_queue* queue)
{
   pthread_mutex_lock(&queue->mutex);
   while(!queue->has_jobs)
      pthread_cond_wait(&queue->has_jobs_cond ,&(queue->mutex));
   queue->has_jobs = false;
   pthread_mutex_unlock(&queue->mutex);
}



void job_queue_free(struct job_queue* queue)
{
   job_queue_clear(queue);
   free(queue);
}



/*                                   Private function                                 */

void notify_jobs(struct job_queue* queue)
{
   pthread_mutex_lock(&queue->mutex);
   queue->has_jobs = true;
   pthread_cond_signal(&queue->has_jobs_cond);
   pthread_mutex_unlock(&queue->mutex);
}

/*                                   Private function                                 */
