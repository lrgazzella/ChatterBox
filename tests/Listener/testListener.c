/** @file testListener.c
  * @author Lorenzo Gazzella 546890
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
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

    char * str = malloc(sizeof(char) * 5);
    strcpy(str, "ciao");

    message_t m1, m2;
    m2.hdr.op = -1;
    setHeader(&(m1.hdr), REGISTER_OP, "Lorenzo");
    setData(&(m1.data), "", NULL, 0);
    sendRequest(fd, &m1);
    readMsg(fd, &m2);
    printf("Il server ha risposto con codice: %d\n", m2.hdr.op);
    int nusers = m2.data.hdr.len / (MAX_NAME_LENGTH+1);
    assert(nusers > 0);
    printf("Lista utenti online:\n");
    for(int i=0,p=0;i<nusers; ++i, p+=(MAX_NAME_LENGTH+1)) {
        printf(" %s\n", &m2.data.buf[p]);
    }

    printf("STRINGA: |");
    for(int i=0; i<m2.data.hdr.len; i++)
        printf("%c ", m2.data.buf[i]);
    printf("|\n");
    close(fd);
}
