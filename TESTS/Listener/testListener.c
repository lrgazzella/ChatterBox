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
#define SOCKNAME1 "/tmp/chatty_socket"

int main(int argc, char** argv) {

    int fd;
    ec_meno1(fd = openConnection(SOCKNAME1, 10, 1), "Errore openConnection");
    printf("Client connesso\n");
    message_t m1;
    m1.hdr.op = REGISTER_OP;
    strcpy(m1.hdr.sender, "Gianni");
    char * str = malloc(sizeof(char) * 5);
    strcpy(str, "ciao");
    m1.data.hdr.len = 5;
    m1.data.buf = str;
    ec_meno1(sendRequest(fd, &m1), "Errore sendRequest");
    ec_meno1(readMsg(fd, &m1), "Errore readMsg");
    printf("Il server risponde con OP: %d -> %s\n", m1.hdr.op, m1.data.buf);
    m1.hdr.op = CONNECT_OP;
    strcpy(m1.hdr.sender, "Gianni");
    ec_meno1(sendRequest(fd, &m1), "Errore sendRequest");
    ec_meno1(readHeader(fd, &(m1.hdr)), "Errore readHeader");
    printf("Il server risponde con OP: %d\n", m1.hdr.op);
    free(str);
    close(fd);
}
