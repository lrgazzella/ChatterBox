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
    ec_meno1_return(fd_skt = socket(AF_UNIX, SOCK_STREAM, 0), "Errore creazione socket"); // creo la socket

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
    int r = readn(connfd, hdr, sizeof(message_hdr_t));
    //printf("R: %d\n", r);
    if(r <= 0){
        return r;
    }
    return 1;
}

int readData(long fd, message_data_t *data){
    int r = readn(fd, &(data->hdr), sizeof(message_data_hdr_t));
    if(r <= 0){
        return r;
    }
    data->buf = malloc(sizeof(char) * data->hdr.len);
    if(!data->buf) return -1;
    r = readn(fd, data->buf, sizeof(char) * data->hdr.len);
    if(r <= 0){
        return r;
    }
    return 1;
}

int readMsg(long fd, message_t *msg){
    int r = readHeader(fd, &(msg->hdr));
    if(r <= 0) return r;
    return readData(fd, &(msg->data));
}

int sendHeader(long fd, message_hdr_t *msg){
    int r = writen(fd, msg, sizeof(message_hdr_t));
    if(r <= 0) return r;
    return 1;
}

int sendData(long fd, message_data_t *msg){
    int r = writen(fd, &(msg->hdr), sizeof(message_data_hdr_t));
    if(r <= 0){
        return r;
    }
    r = writen(fd, msg->buf, sizeof(char) * msg->hdr.len);
    if(r <= 0){
        return r;
    }
    return 1;
}

int sendRequest(long fd, message_t *msg){
    int r = sendHeader(fd, &(msg->hdr));
    if(r <= 0) return -1;
    r = sendData(fd, &(msg->data));
    if(r<=-1) return -1;
    return r;
}
