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

int main(int argc, char** argv) {

    int fd, fd_c;
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
    free(str);
    sleep(3);
}
