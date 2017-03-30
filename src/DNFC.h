/*H**********************************************************************
* FILENAME :        ABS_DNFC.h
*
* DESCRIPTION :
*        Dynamic flow calssification API.
*
* PUBLIC FUNCTIONS :
*       int    DNFC_init( struct DNFC_conf *, char *, char *)
*       int    DNFC_close( struct DNFC_conf *)
*       void  DNF_process( struct DNFC_conf *)
*
* PUBLIC STRUCTURE :
*       struct DNFC_conf
*
* NOTES :
*       These functions use the netmap API to perform fast packet retreival.
*
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdio.h>
#include <stdbool.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>
#include <sys/poll.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>

#include "../libs/memory_management/memory_management.h"


#define OPEN                          1
#define CLOSE                        2
#define RUNNING                    3
#define STOP                          4
#define ERROR_IFACE              5
#define ERROR_FREE              -1
#define PROCESS_MAIL_BOX   1200



/*******************************************************************
* NAME :                 struct DNFC_conf
*
* DESCRIPTION :      Structure that hold all the necessarily informations to perform
*                             the dynamic classification
*
* MEMBERS:
*           char *iface                           The name of the physical interface
*           struct nm_desc *desc          The netmap descriptor file
*           struct pollfd *fds                  The filedescriptor of the netmap device
*           bool state                             The state of the classifier: RUNNING || STOP
*           int msgq_id                          The message queue id to get messages
*
*******************************************************************/
struct DNFC_conf
{
   char *iface;
   struct nm_desc *desc;
   struct pollfd *fds;
   bool  state;
   int msgq_id;
};



/*******************************************************************
* NAME :                 struct DNFC
*
* DESCRIPTION :      Structure that hold all the method of the algorithm
*                             to perform the classification.
* MEMBERS:
*           int (*init)();            The initialisation of the algorithm (a default implementation is given )
*           int (*process)();     The process function that classify incoming packets.
*           int (*close)();         The close function
*
*******************************************************************/
struct DNFC
{
   int (*init)(struct DNFC_conf *, char *, int);
   void (*process)(struct DNFC_conf *);
   int (*close)(struct DNFC_conf *);
};



/*******************************************************************
* NAME :                 int DNFC_init(struct DNFC_conf *output, char *iface, char *port)
*
* DESCRIPTION :      Produce a configuration structure used to classify network flows.
*                             The structure contain a pointer to a file descriptor on which the operations
*                             are based.
*
* INPUTS :
*       PARAMETERS:
*           char *iface            The name of the interface on witch the classification is performed
*           int port                 The port of the flow to catch
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*           struct DNFC_conf *     A structure that contain a pointer to the file descriptor bounded to the specified NIC
*       GLOBALS :
*            None
*       RETURN :
*            Type:      int                            Error code:
*            Values:   OPEN                        valid initialization parameters
*                          ERROR_IFACE            invalid specified interface
* PROCESS :
*                   [1]  Check the interface availability
*                   [2]  Bound a file descriptor the specified NIC
*
* NOTES :      As soon the init function is called with correct parameters the data path of the NIC is
*                   disconnected from the host stack as specified in netmap.
*                   Still prototyping, may not be complete and trying to be genertic and easy to use.
*******************************************************************/
int DNFC_init(struct DNFC_conf *output, char *iface, int port);



/*******************************************************************
* NAME :                 int DNFC_close(struct DNFC_conf *conf,)
*
* DESCRIPTION :      End the dynamic flow classifier and free the memory.
*
* INPUTS :
*       PARAMETERS:
*           struct DNFC_conf* conf           The configuration structure of the classifier
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*            None
*       RETURN :
*            Type:      int                            Error code:
*            Values:   CLOSED                     DNFC ended
*                          ERROR_IFACE            cannot close the interface
*                          ERROR_FREE              empty structure pointer
* PROCESS :
*                   [1]  Close the interface
*                   [2]  Free the memory
*
* NOTES :      None
*******************************************************************/
int DNFC_close(struct DNFC_conf *conf);
