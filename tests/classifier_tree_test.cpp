#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../src/classifiers/classifier_tree/classifier_tree.h"



void time_measurment(struct classifier_tree** tree, uint32_t size, struct network_flow* flows);
struct network_flow* get_random_flows(uint32_t size);
uint32_t rand32(void);
uint8_t rand_protocol(void);


int main(void)
{
   uint32_t size = 50000;
   srand((unsigned int)time(NULL));
   struct network_flow* flows = get_random_flows(size);
   struct classifier_tree* tree = new_tree(UDIET, 10);

   fprintf(stderr, "\nUDIET\n");
   time_measurment(&tree, size, flows);

   fprintf(stderr, "\nMurmur3:\n");
   tree = new_tree(MURMUR, 10);
   time_measurment(&tree, size, flows);

   fprintf(stderr, "\nFNV-1a:\n");
   tree = new_tree(FNV_1, 10);
   time_measurment(&tree, size, flows);

   free(flows);
   return(0);
}


void time_measurment(struct classifier_tree** tree, uint32_t size, struct network_flow* flows)
{
   struct timeval start;
   struct timeval end;
   struct timeval elapsed;

   gettimeofday(&start, NULL);
   for (uint32_t i = 0; i < size; ++i)
   {
      classifier_tree_add(tree, &flows[i]);
   }
   gettimeofday(&end, NULL);
   timersub(&end, &start, &elapsed);
   fprintf(stderr, "Put %u elements in %ld.%06ld seconds\n", size, (long int)elapsed.tv_sec, (long int)elapsed.tv_usec);


   uint32_t count_remove = 0;
   gettimeofday(&start, NULL);
   for(uint32_t m = 0; m < size; ++m)
   {
      classifier_tree_delete(*tree, &flows[m]);
      count_remove++;
   }
   gettimeofday(&end, NULL);
   timersub(&end, &start, &elapsed);
   fprintf(stderr, "Remove: %u elements in %ld.%06ld seconds\n", count_remove, (long int)elapsed.tv_sec, (long int)elapsed.tv_usec);


   uint32_t count_contain = 0;
   gettimeofday(&start, NULL);
   for(uint32_t k = 0; k < size; ++k)
   {
      if(classifier_tree_contain(*tree, &flows[k]))
         count_contain++;
   }
   gettimeofday(&end, NULL);
   timersub(&end, &start, &elapsed);
   fprintf(stderr, "Contained: %u (%u) elements in %ld.%06ld seconds\n", count_contain, size, (long int)elapsed.tv_sec, (long int)elapsed.tv_usec);
}



struct network_flow* get_random_flows(uint32_t size)
{
   struct network_flow* result = malloc((sizeof *result) * size);
   for(uint32_t i = 0; i < size; i++)
   {
      result[i].protocol = rand_protocol();
      result[i].source_address = rand32();
      result[i].destination_address = rand32();      
   }
   return result;
}



uint32_t rand32(void)
{
      uint32_t rslt;
      rslt = rand() & 0xff;
      rslt |= (rand() & 0xff) << 8;
      rslt |= (uint32_t)((rand() & 0xff) << 16);
      rslt |= (uint32_t)((rand() & 0xff) << 24);
      return rslt;
}



uint8_t rand_protocol(void)
{
   if((rand() % 2) == 1)
   {
      return (uint8_t) 6;
   } else {
      return (uint8_t) 17;
   }
}
