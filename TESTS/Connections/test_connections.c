#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "../../connections.h"
#include "../../ops.h"
#include "../../message.h"
#include "../../utility.h"
#define SOCKNAME1 "./mysock"
/*
*
*/
int main(int argc, char** argv) {

  int fd, fd_skt, fd_c;
  if(fork() == 0){ //client
    ec_meno1(fd = openConnection(SOCKNAME1, 10, 1), "Errore openConnection");
    printf("Client connesso\n");
    message_t m1;
    m1.hdr.op = POSTTXT_OP;
    strcpy(m1.hdr.sender, "Gianni");
    strcpy(m1.data.hdr.receiver, "Paolo");
    char * str = malloc(sizeof(char) * 5);
    strcpy(str, "ciao");
    m1.data.hdr.len = 5;
    m1.data.buf = str;
    ec_meno1(sendRequest(fd, &m1), "Errore sendRequest");
  }else{ //server
    sleep(5);
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME1, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    ec_meno1(fd_skt = socket(AF_UNIX,SOCK_STREAM,0), "Errore socket");
    ec_meno1(bind(fd_skt,(struct sockaddr *)&sa, sizeof(sa)), "Errore bind");
    ec_meno1(listen(fd_skt, SOMAXCONN), "Errore listen");
    message_t m2;
    ec_meno1(fd_c = accept(fd_skt,NULL,0), "Errore accept");
    printf("Server riceve client\n");
    ec_meno1(readMsg(fd_c, &m2), "Errore readMsg");
    printf("Sender: %s\n", m2.hdr.sender);
    printf("Receiver: %s\n", m2.data.hdr.receiver);
    printf("Messaggio: %s\n", m2.data.buf);
    printf("Operazione: %d\n", m2.hdr.op);
    printf("Len: %d\n", m2.data.hdr.len);
    free(m2.data.buf);
  }

}
