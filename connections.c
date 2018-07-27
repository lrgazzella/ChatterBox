#include "connections.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ops.h"
#include "config.h"
#include "utility.h"



int openConnection(char* path, unsigned int ntimes, unsigned int secs){
    int fd_skt;
    ec_meno1_return(socket(AF_UNIX, SOCK_STREAM, 0), "Errore creazione socket"); // creo la socket

    struct sockaddr_un sa;
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    struct timespec req = {secs, 0};
    struct timespec rem;

    int i = 0;
    for(i=0; i<ntimes; i++){
        if(connect(fd_skt, (struct sockaddr*) &sa, sizeof(sa)) != -1)
            return fd_skt;
        printf("Aspetto il server: %d\n", i);
        while(nanosleep(&req, &rem) == -1){
          if(errno == EINTR)
            req = rem;
          else return -1;
        }

    }
    return -1;
}

int readHeader(long connfd, message_hdr_t *hdr){ //message_hdr_t ha op e sender
  ec_meno1_return(readn(connfd, hdr, sizeof(message_hdr_t)), "Errore lettura header");
  return 0;
}

int readData(long fd, message_data_t *data){
  ec_meno1_return(readn(fd, &(data->hdr), sizeof(message_data_hdr_t)), "Errore lettura header del body");
  data->buf = malloc(sizeof(char) * (data->hdr.len + 1));
  ec_meno1_return(readn(fd, data->buf, sizeof(char) * (data->hdr.len + 1)), "Errore lettura buffer");
  return 0;
}

int readMsg(long fd, message_t *msg){
  ec_meno1_return(readHeader(fd, &(msg->hdr)), "Errore lettura header");
  ec_meno1_return(readData(fd, &(msg->data)), "Errore lettura body");
}

int sendData(long fd, message_data_t *msg){
  ec_meno1_return(writen(fd, &(msg->hdr), sizeof(message_data_hdr_t)), "Errore scrittura header del body");
  ec_meno1_return(writen(fd, msg->buf, sizeof(char) * (msg->hdr.len + 1)), "Errore scrittura buffer");
  return 0;
}

int sendRequest(long fd, message_t *msg){
  ec_meno1_return(writen(fd, &(msg->hdr), sizeof(message_hdr_t)), "Errore scrittura header");
  ec_meno1_return(sendData(fd, &(msg->data)), "Errore scrittura body");
  return 0;
}
