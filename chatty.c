/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "stats.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "./GestioneQueue/queue.h"
#include "./utility.h"
#include "./connections.h"
#include "./parser.h"
#include <sys/select.h>

static Queue_t * richieste;
static config configurazione;
static int nSocketUtenti = 0;

int createSocket();
int aggiorna(fd_set * set, int max);
int isPipe(int fd);

struct statistics chattyStats = { 0,0,0,0,0,0,0 };

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

static void* listener(void* arg);

int main(int argc, char *argv[]) {

    /* TEST */
    initParseCheck("../../DATA/chatty.conf1", &configurazione);
    richieste = initQueue();
    int fd_skt = createSocket();
    pthread_t tid;
    pthread_create(&tid, NULL, &listener, NULL);
    pthread_join(tid, NULL);

}

static void* listener(void* arg){
    int fd_skt, fd_num = 0;
    fd_set rdset, set;
    if((fd_skt = createSocket()) == -1){
        // TODO gestione errore
    }
    if(fd_skt > fd_num) fd_num = fd_skt;
    FD_ZERO(&set);
    FD_SET(fd_skt,&set);
    while (1) {
        rdset = set;
        if (select(fd_num+1,&rdset,NULL,NULL,NULL) == -1) {
            perror("Errore select");
            exit(-1); // TODO gestione errore
        } else {
            int fd;
            for (fd = 0; fd<=fd_num;fd++) {
                if (FD_ISSET(fd, &rdset)) {
                    if (fd == fd_skt) { /* sock connect pronto */
                        int fd_c = accept(fd_skt, NULL, 0);
                        if(nSocketUtenti >= configurazione.MaxConnections){
                            message_t m;
                            setHeader(&(m.hdr), OP_FAIL, "");
                            setData(&(m.data), "", "Troppi utenti collegati", 24);
                            sendRequest(fd_c, &m);
                            close(fd_c);
                        } else {
                            printf("Un client si è connesso\n");
                            nSocketUtenti ++;
                            FD_SET(fd_c, &set);
                            if (fd_c > fd_num) fd_num = fd_c;
                        }
                    } else {
                        int p;
                        if((p = isPipe(fd)) == 1){ // un thread del pool mi sta comunicando che un fd è di nuovo disponibile
                            int fd_c;
                            if(readn(fd, &fd_c, sizeof(int)) == -1){ //TODO gestione errore
                                perror("Errore read su pipe");
                                exit(-1);
                            }
                            FD_SET(fd_c, &set);
                            if (fd_c > fd_num) fd_num = fd_c;
                        }else if(p == 0){ // un client mi sta mandando una richiesta
                            printf("Un client ha fatto una richiesta\n");
                            if(push(richieste, &fd) == -1){ // TODO gestione errore
                                perror("Errore push");
                                exit(-1);
                            }
                            FD_CLR(fd, &set);
                            fd_num = aggiorna(&set, fd_num);
                        }else{
                            // TODO gestione errore
                            perror("Errore isPipe");
                            exit(-1);
                        }
                    }
                }
            }
        }
    }
}

/**
* @return: -1 se ci sono stati errori
*           1 se fd è una pipe
*           0 altrimenti
*/
int isPipe(int fd){
    struct stat file;
    ec_meno1_return(fstat(fd, &file), "Errore stat file");
    return S_ISFIFO(file.st_mode);
}

int aggiorna(fd_set * set, int max){
    int fd, r = 0;
    for(fd=0; fd<=max; fd++){
        if(FD_ISSET(fd, set)){
            r = fd;
        }
    }
    return r;
}

// legge file configurazione, eliminare l'eventuale socket vecchia e crea la socket del server
int createSocket(){
    if(remove(configurazione.UnixPath) == -1){
        if(errno != ENOENT) {// Se errno == ENOENT vuol dire che non esiste un file con quel path
            return -1;
        }
    }
    int fd_skt;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, configurazione.UnixPath, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    ec_meno1_return(fd_skt = socket(AF_UNIX,SOCK_STREAM,0), "Errore socket");
    ec_meno1_return(bind(fd_skt,(struct sockaddr *)&sa, sizeof(sa)), "Errore bind");
    ec_meno1_return(listen(fd_skt, SOMAXCONN), "Errore listen");
    return fd_skt;
}
