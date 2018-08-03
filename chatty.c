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
#include <sys/select.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include "./struttureCondivise.h"
#include "./parser.h"
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneListe/list.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./lib/GestioneBoundedQueue/boundedqueue.h"

#include "./utility.h"
#include "./connections.h"
#include "./config.h"
#include "./gestioneRichieste.h"


/* Variabili globali */
Queue_t * richieste;
config configurazione;
hash_s * utentiRegistrati;
list_s * utentiConnessi;
connessi_s * nSock;


/* Definizione funzioni */
int createSocket();
int aggiorna(fd_set * set, int max);
int isPipe(int fd);
static void * listener(void * arg);
static void * pool(void * arg);
static void * segnali(void * arg);
void initHashLock();
void printRisOP(message_t m, int ok);


/* Funzione di hash */
static inline int compareString( void *key1, void *key2  ) {
    return !strcmp((char *) key1, (char *) key2);
}

struct statistics chattyStats = { 0,0,0,0,0,0,0 };

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}


int main(int argc, char *argv[]) {
    /* Prendo il file di configurazione */
    int optc;
    const char optstring[] = "f:";
    char * pathFileConf = NULL;// TODO va deallocata?
    while ((optc = getopt(argc, argv, optstring)) != -1) {
        switch (optc) {
            case 'f':
                pathFileConf = optarg;
            break;
            default:
                usage(argv[0]);
                return -1;
        }
    }
    if(pathFileConf == NULL){
        usage(argv[0]);
        return -1;
    }
    int i;

    /* Inizializzazione */
    initParseCheck(pathFileConf, &configurazione);//TODO controllare errori
    richieste = initQueue();
    nSock = malloc(sizeof(connessi_s));
    nSock->contatore = 0;
    pthread_mutex_init(&(nSock->contatore_m), NULL);
    utentiRegistrati = malloc(sizeof(hash_s));
    ec_null_return(utentiRegistrati->hash = icl_hash_create(HASHSIZE, NULL, compareString), "Errore creazione hash");
    utentiConnessi = malloc(sizeof(list_s));
    ec_null_return(utentiConnessi->list = list_new(), "Errore creazione lista");
    pthread_mutex_init(&(utentiConnessi->list_m), NULL);
    utentiConnessi->list->free = freeUtente_Connesso_s;
    initHashLock();

    /* Creazione pipe */
    int ** pfds = malloc(sizeof(int *) * configurazione.ThreadsInPool);
    for(i=0; i<configurazione.ThreadsInPool; i++){
        pfds[i] = malloc(sizeof(int) * 2);
        if(pipe(pfds[i]) == -1){ //TODO gestione errore
            perror("Errore pipe\n");
            exit (-1);
        }
    }

    /* Gestione segnali */

    /* Avvio listener */
    pthread_t listener_id;
    pthread_create(&listener_id, NULL, &listener, pfds);

    /* Avvio pool */
    pthread_t * pool_id = malloc(sizeof(pthread_t) * configurazione.ThreadsInPool); // TODO gestione Errore
    for(i=0; i<configurazione.ThreadsInPool; i++){
        pthread_create(&(pool_id[i]), NULL, &pool, &(pfds[i][1]));
    }

    /* Join */
    int * ret_listener;
    pthread_join(listener_id, (void **)&ret_listener);
    int * ret_pool;
    for(i=0; i<configurazione.ThreadsInPool; i++){
        pthread_join(pool_id[i], (void **)&ret_pool);
    }
    free(pathFileConf);
    return 0;
}


void initHashLock(){
    int i=0;
    for(i=0; i<(HASHSIZE / HASHGROUPSIZE); i++){
        pthread_mutex_init(&(utentiRegistrati->hash_m[i]), NULL);
    }
}

static void * pool(void * arg){
    int * fd;
    int pipe = *((int *)arg);
    int ris, r;
    message_t msg;
    while(1){
        fd = (int *)pop(richieste);
        r = readMsg(*fd, &msg);
        if(r < 0){ //TODO controllare errori. Non necessita di lock fd dato che un utente può fare una richiesta alla volta che viene presa in gestione da un solo thread del pool
            free(fd);
            perror("Errore readMsg");
            exit(-1);
        }else if(r == 0){
            // vuol dire che il client ha finito di comunicare, allora devo chiudere la connessione e eliminare l'utente dagli utenti connessi
            // è come se mi mandasse un messaggio con operazione DISCONNECT_OP
            // oss: Devo controllare se esiste nella lista degli utenti un utente_connesso_s->fd = *fd
            LOCKList();
            disconnect_op(*fd); // rimuovo l'utente dalla lista degli utenti connessi (se esisite)
            UNLOCKList();
            close(*fd); // Chiudo il socket. TODO controllare errore
            LOCKnSock();
            nSock->contatore --;
            UNLOCKnSock();
        }else{
            printf("Ricevuta op: %d da: %s\n", msg.hdr.op, msg.hdr.sender);
            switch(msg.hdr.op){
                case REGISTER_OP:
                    LOCKHash(msg.hdr.sender);
                    ris = register_op(*fd, msg); //TODO controllare errori
                    UNLOCKHash(msg.hdr.sender);
                    printRisOP(msg, ris);
                    break;
                case CONNECT_OP:
                    LOCKHash(msg.hdr.sender);
                    LOCKList();
                    ris = connect_op(*fd, msg);
                    UNLOCKList();
                    UNLOCKHash(msg.hdr.sender);
                    printRisOP(msg, ris);
                    break;
                /*case POSTTXT_OP:
                    posttxt_op(msg, utentiRegistrati, hashLock, utentiConnessi);
                    break;
                case POSTTEXTALL_OP:
                    posttextall_op(msg, utentiRegistrati, hashLock, utentiConnessi);
                    break;
                case POSTFILE_OP:
                    postfile_op(msg, utentiRegistrati, hashLock, utentiConnessi);
                    break;
                case GETFILE_OP:
                    getfile_op(msg, utentiRegistrati, hashLock, utentiConnessi);
                    break;
                case GETPREVMSGS_OP:
                    getprevmsgs_op(msg, utentiRegistrati, hashLock, utentiConnessi);
                    break;*/
                case USRLIST_OP:
                    LOCKList();
                    ris = usrlist_op(*fd, msg);
                    UNLOCKList();
                    printRisOP(msg, ris);
                    break;
                case UNREGISTER_OP:
                    LOCKHash(msg.hdr.sender);
                    LOCKList();
                    ris = unregister_op(*fd, msg);
                    UNLOCKList();
                    UNLOCKHash(msg.hdr.sender);
                    printRisOP(msg, ris);
                    break;
                default: printf("Errore default\n");
                    //TODO gestione errore
            }
            if(writen(pipe, fd, sizeof(int)) == -1){
                perror("Errore writen");
                exit(-1);//TODO controlla errori
            }
        }
        free(msg.data.buf);
        free(fd);
    }
}

void printRisOP(message_t m, int ok){
    switch(m.hdr.op){
        case REGISTER_OP:
            if(ok == 0) printf("REGISTRAZIONE COMPLETATA: %s\n", m.hdr.sender);
            else printf("ERRORE. REGISTRAZIONE ANNULLATA\n");
            break;
        case CONNECT_OP:
            if(ok == 0) printf("UTENTE CONNESSO: %s\n", m.hdr.sender);
            else printf("ERRORE. CONNESSIONE ANNULLATA");
            break;
        case USRLIST_OP:
            if(ok == 0) printf("LISTA UTENTI ONLINE SPEDITA\n");
            else printf("ERRORE. SPEDIZIONE ANNULLATA\n");
            break;
        case UNREGISTER_OP:
            if(ok == 0) printf("UTENTE ELIMINATO\n");
            else printf("ERRORE. ELIMINAZIONE UTENTE ANNULLATA");
    }
}

static void * listener(void * arg){
    int fd_skt, fd_num = 0;
    int i;
    fd_set rdset, set;
    if((fd_skt = createSocket()) == -1){
        printf("Errore creazione socket\n");
        exit(-1);
        // TODO gestione errore
    }
    int **pfds = (int **)arg;
    FD_ZERO(&set);
    for(i=0; i<configurazione.ThreadsInPool; i++){
        FD_SET(pfds[i][0], &set);
        if(pfds[i][0] > fd_num) fd_num = pfds[i][0];
    }
    FD_SET(fd_skt,&set);
    if(fd_skt > fd_num) fd_num = fd_skt;
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
                        pthread_mutex_lock(&(nSock->contatore_m));
                        if(nSock->contatore >= configurazione.MaxConnections){
                            pthread_mutex_unlock(&(nSock->contatore_m));
                            message_t m;
                            setHeader(&(m.hdr), OP_FAIL, "");
                            setData(&(m.data), "", "Troppi utenti collegati", 24);
                            sendRequest(fd_c, &m);
                            close(fd_c);
                        } else {
                            nSock->contatore ++;
                            pthread_mutex_unlock(&(nSock->contatore_m));
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
                            int * toPush = malloc(sizeof(int));
                            *toPush = fd;
                            if(push(richieste, toPush) == -1){ // TODO gestione errore
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

static void * segnali(void * arg){}
/**
* @return: -1 se ci sono stati errori
*           1 se fd è una pipe
*           0 altrimenti
*/
int isPipe(int fd){
    if(fd < 0) return -1;
    struct stat file;
    if(fstat(fd, &file) == -1 ) return -1;
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
    if((fd_skt = socket(AF_UNIX,SOCK_STREAM,0)) == -1) return -1;
    ec_meno1_return(bind(fd_skt,(struct sockaddr *)&sa, sizeof(sa)), "Errore bind");
    ec_meno1_return(listen(fd_skt, SOMAXCONN), "Errore listen");
    return fd_skt;
}
