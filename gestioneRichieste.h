#ifndef GESTIONE_RICHIESTE_
#define GESTIONE_RICHIESTE_

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include "message.h"
#include "ops.h"
#include "./config.h"
#include "./struttureCondivise.h"
#include "./gestioneRichieste.h"
#include "./chatty.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"
#include "./lib/GestioneHistory/codaCircolare.h"


int register_op(long fd, message_t m); // una volta che mi registro sono pure connesso
int connect_op(long fd, message_t m, int atomica);
int posttxt_op(long fd, message_t m);
int posttxtall_op(long fd, message_t m);
int postfile_op(long fd, message_t m);
int getfile_op(long fd, message_t m);
int getprevmsgs_op(long fd, message_t m);
int usrlist_op(long fd, message_t m, int atomica);
int unregister_op(long fd, message_t m);
int disconnect_op(long fd);


#endif
