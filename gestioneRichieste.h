#include <stdio.h>
#include <stdlib.h>
#include "./config.h"

int register_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati,  linked_list_t * connessi); // una volta che mi registro sono pure connesso
int connect_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int posttxt_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int posttextall_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int postfile_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int getfile_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int getprevmsgs_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int usrlist_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int unregister_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
int disconnect_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati, linked_list_t * connessi);
