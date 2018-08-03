#ifndef GESTIONE_RICHIESTE_
#define GESTIONE_RICHIESTE_

#include <stdio.h>
#include <stdlib.h>
#include "message.h"
#include "ops.h"
#include "./config.h"
#include "./struttureCondivise.h"



int register_op(long fd, message_t m); // una volta che mi registro sono pure connesso
int connect_op(long fd, message_t m);
int posttxt_op(long fd, message_t m);
int posttextall_op(long fd, message_t m);
int postfile_op(long fd, message_t m);
int getfile_op(long fd, message_t m);
int getprevmsgs_op(long fd, message_t m);
int usrlist_op(long fd, message_t m);
int unregister_op(long fd, message_t m);
int disconnect_op(long fd);


#endif
