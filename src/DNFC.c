#include <stdio.h>
#include "ABS_DNFC.h"
#include "lib/classifiers/naive/naive_classifier.h"


int main(int argc, char **argv)
{
   struct DNFC_conf *classifier_conf = NULL;
   char *iface = NULL;
   int port;
   int options;

   /* Getting the parameters */
   while((options = getopt(argc, argv, "i:p:")) != -1)
   {
      switch(options)
      {
         case 'i':
            iface = optarg;
            break;
         case 'p':
            port = atoi(optarg);
            break;
         default:
            D("Bad usage");
            return(1);
      }
   }

   /*Open the netmap interface*/
   struct DNFC *classifier = new_pc();

   if(classifier->init(classifier_conf, iface, port) > 0)
   {
      D("Opened");
      classifier->process(classifier_conf);
      //classifier->close(classifier_conf);
      D("Closed");
   }
   return(0);
}



int DNFC_init(struct DNFC_conf *output, char *iface, int port)
{
   struct nm_desc *piface = NULL;

   /* [1][2] */
   piface = nm_open(iface, NULL, 0, 0);
   if(piface == NULL)
   {
      return(ERROR_IFACE);
   } else {
      /* File descriptor creation */
      struct pollfd *fds = chkmalloc(sizeof *fds);
      fds->fd = NETMAP_FD(piface);
      fds->events = POLLIN;

      output = chkmalloc(sizeof *output);
      output->desc = chkmalloc(sizeof output->desc);
      output->ds_map = chkmalloc(sizeof output->ds_map);
      output->iface = chkmalloc(sizeof *iface);
      output->fds = chkmalloc(sizeof *fds);

      output->desc = piface;
      output->iface = iface;
      output->fds = fds;
      output->port = port;
      output->state = RUNNING;
      output->msgq_id = msgget(PROCESS_MAIL_BOX, 0600 | IPC_CREAT);
      return(OPEN);
   }
}



int DNFC_close(struct DNFC_conf *conf)
{
   if(conf)
   {
      /*[1]*/
      nm_close(conf->desc);
      /*[2]*/
      if(conf->desc)
      {
         free(conf->desc);
      }
      if (conf->ds_map)
      {
         munmap(conf->ds_map, conf->ds_size);
         free(conf->ds_map);
      }
      if (conf->iface)
      {
         free(conf->iface);
      }
      if (conf->fds)
      {
         free(conf->fds);
      }
      free(conf);
      return(CLOSE);
   }
   return(ERROR_FREE);
}
