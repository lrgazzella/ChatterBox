#include <stdio.h>
#include <stdlib.h>
#include "./config.h"

typedef struct utente_connesso{
    char nickname[MAX_NAME_LENGTH];
    long fd;
    pthread_mutex_t fd_m;
}, utente_connesso_s;

int register_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati,  linked_list_t * connessi); // una volta che mi registro sono pure connesso
int connect_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int posttxt_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int posttextall_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int postfile_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int getfile_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int getprevmsgs_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int usrlist_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int unregister_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int disconnect_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
