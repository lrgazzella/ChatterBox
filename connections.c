/** @file connections.c
  * @author Lorenzo Gazzella 546890
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#define _POSIX_C_SOURCE 199309L
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
    if(path == NULL) return -1;

    int fd_skt;
    if((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) return -1;


    struct sockaddr_un sa;
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    struct timespec req = {secs, 0};
    struct timespec rem;

    int i;
    for(i=0; i<ntimes; i++){
        if(connect(fd_skt, (struct sockaddr*) &sa, sizeof(sa)) != -1)
            return fd_skt;
        printf("Errore. Provo a riconnettrmi: %d\n", i);
        while(nanosleep(&req, &rem) == -1){
            if(errno == EINTR)
                req = rem;
            else return -1;
        }

    }
    return -1;
}

int readHeader(long fd, message_hdr_t *hdr){
    if(fd < 0 || hdr == NULL) return -1;
    return readn(fd, hdr, sizeof(message_hdr_t));
}

int readData(long fd, message_data_t *data){
    if(fd < 0 || data == NULL) return -1;
    int r = readn(fd, &(data->hdr), sizeof(message_data_hdr_t));
    if(r <= 0){
        return r;
    }
    if(data->hdr.len == 0){
        data->buf = NULL;
        return r;
    }else{
        data->buf = calloc(data->hdr.len, sizeof(char));
        if(!data->buf) return -1;
        return readn(fd, data->buf, sizeof(char) * data->hdr.len);
    }
}

int readMsg(long fd, message_t *msg){
    if(fd < 0 || msg == NULL) return -1;
    int r = readHeader(fd, &(msg->hdr));
    if(r <= 0) return r;
    return readData(fd, &(msg->data));
}

int sendHeader(long fd, message_hdr_t *hdr){
    if(fd < 0 || hdr == NULL) return -1;
    //bzero(hdr, sizeof(message_hdr_t));
    return writen(fd, hdr, sizeof(message_hdr_t));
}

int sendData(long fd, message_data_t *data){
    if(fd < 0 || data == NULL) return -1;
    int r = writen(fd, &(data->hdr), sizeof(message_data_hdr_t));
    if(r <= 0){
        return r;
    }
    if(data->hdr.len != 0) // se il buf è lungo 0 vuol dire che buf == NULL. Allora non lo invio
        return writen(fd, data->buf, sizeof(char) * data->hdr.len);
    return r;
}

int sendRequest(long fd, message_t *msg){
    if(fd < 0 || msg == NULL) return -1;
    int r = sendHeader(fd, &(msg->hdr));
    if(r <= 0) return -1;
    return sendData(fd, &(msg->data));
}
