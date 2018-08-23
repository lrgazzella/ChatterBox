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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
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
#include "./stats.h"
#include "./utility.h"
#include "./connections.h"
#include "./config.h"
#include "./gestioneRichieste.h"


/* Variabili globali */
Queue_t * richieste;
config * configurazione;
hash_s * utentiRegistrati;
array_s * utentiConnessi;
connessi_s * nSock;
stat_s * statistiche;
pthread_t * allThread;


/* Definizione funzioni */
int createSocket();
int aggiorna(fd_set * set, int max);
int isPipe(int fd);
static void * listener(void * arg);
static void * pool(void * arg);
static void * segnali(void * arg);
void initHashLock();
void initRegistrati();
void initDirFile();
void initNSock();
void initStat();
void initConnessi();
void printRisOP(message_t m, int ok);
void stopAllThread();


/* Funzione di hash */
static inline int compareString( void *key1, void *key2  ) {
    return !strcmp((char *) key1, (char *) key2);
}


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
    configurazione = malloc(sizeof(config));
    initParseCheck(pathFileConf, configurazione); //TODO controllare errori
    initDirFile();
    richieste = initQueue();
    initStat();
    initNSock();
    initRegistrati(); //TODO controllare errori
    initConnessi();
    allThread = malloc(sizeof(pthread_t) * (configurazione->ThreadsInPool + 2)); // pool + listener + segnali

    /* Creazione pipe */
    int ** pfds = malloc(sizeof(int *) * configurazione->ThreadsInPool);
    for(i=0; i<configurazione->ThreadsInPool; i++){
        pfds[i] = malloc(sizeof(int) * 2);
        if(pipe(pfds[i]) == -1){ //TODO gestione errore
            perror("Errore pipe\n");
            exit (-1);
        }
    }

    /* Gestione segnali */

    /* Ignoro SIGPIPE per tutti */
    struct sigaction toIgnore;
    memset(&toIgnore, 0, sizeof(toIgnore));
    toIgnore.sa_handler = SIG_IGN;
    ec_meno1(sigaction(SIGPIPE, &toIgnore, NULL), "Errore sigaction SIGPIPE");
    /* Setto la maschera per tutti i thread */
    sigset_t toHandle;
    sigemptyset(&toHandle);
    sigaddset(&toHandle, SIGUSR1);
    sigaddset(&toHandle, SIGQUIT);
    sigaddset(&toHandle, SIGTERM);
    sigaddset(&toHandle, SIGINT);
    pthread_sigmask(SIG_BLOCK, &toHandle, NULL);
    /* Avvio thread segnali */
    pthread_t segnali_id;
    pthread_create(&segnali_id, NULL, &segnali, (void *)&toHandle);
    allThread[0] = segnali_id;

    /* Avvio listener */
    pthread_t listener_id;
    pthread_create(&listener_id, NULL, &listener, pfds);
    allThread[1] = listener_id;

    /* Avvio pool */
    pthread_t * pool_id = malloc(sizeof(pthread_t) * configurazione->ThreadsInPool); // TODO gestione Errore
    for(i=0; i<configurazione->ThreadsInPool; i++){
        pthread_create(&(pool_id[i]), NULL, &pool, &(pfds[i][1]));
        allThread[i+2] = pool_id[i];
    }

    /* Join */
    printf("JOIN\n");
    int * ret_listener, * ret_segnali;
    pthread_join(segnali_id, (void **)&ret_segnali); // TODO se tanto nel thread segnali elimino tutti i thread, a che mi serve fare le join sugli altri thread?
    pthread_join(listener_id, (void **)&ret_listener);
    int * ret_pool;
    for(i=0; i<configurazione->ThreadsInPool; i++){
        pthread_join(pool_id[i], (void **)&ret_pool);
    }
    printf("THREAD STOPPATI\n");
    /* FREE */
    // Eliminare la hash e le relative history
    icl_hash_destroy(utentiRegistrati->hash, free, freeCoda);
    for(i=0; i<HASHSIZE / HASHGROUPSIZE; i++){
        pthread_mutex_destroy(&(utentiRegistrati->hash_m[i]));
    }

    free(utentiRegistrati);
    printf("FREE REGISTRATI\n");
    // Eliminare array connessi
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname)
            free(utentiConnessi->arr[i].nickname);
        pthread_mutex_destroy(&(utentiConnessi->arr[i].fd_m));
    }
    free(utentiConnessi->arr);
    pthread_mutex_destroy(&(utentiConnessi->arr_m));
    free(utentiConnessi);
    printf("FREE CONNESSI\n");
    // Eliminare la coda delle richieste
    deleteQueue(richieste); // TODO non libera i nodi
    printf("FREE QUEUE\n");
    // Eliminare le statistiche
    pthread_mutex_destroy(&(statistiche->stat_m));
    free(statistiche);
    printf("FREE STAT\n");
    // Eliminare nSock
    pthread_mutex_destroy(&(nSock->contatore_m));
    free(nSock);
    printf("FREE NSOCK\n");
    // Eliminare pipe
    for(i=0; i<configurazione->ThreadsInPool; i++){
        free(pfds[i]);
    }
    free(pfds);
    printf("FREE PIPE\n");
    // Eliminare Configurazioni
    FreeConfig(configurazione);
    printf("FREE CONFIG\n");
    //free(pathFileConf);
    free(pool_id);
    free(allThread);
    printf("FREE TUTTO\n");
    return 0;
}

void initStat(){
    statistiche = malloc(sizeof(stat_s));
    statistiche->stat.nusers = 0;
    statistiche->stat.nonline = 0;
    statistiche->stat.ndelivered = 0;
    statistiche->stat.nnotdelivered = 0;
    statistiche->stat.nfiledelivered = 0;
    statistiche->stat.nfilenotdelivered = 0;
    statistiche->stat.nerrors = 0;
    pthread_mutex_init(&(statistiche->stat_m), NULL);
}

// Se esiste già la cartella con quel nome la svuota, altrimenti la crea
void initDirFile(){
    DIR * fileDir = NULL;
    size_t lenPath = strlen(configurazione->DirName);

    if((fileDir = opendir(configurazione->DirName)) == NULL){
        if(errno == ENOENT){ // la directory che ho cercato di aprire non esiste
            if(mkdir(configurazione->DirName, 0777) == -1){
                perror("Errore creazione cartella file");
                exit(-1);
            }
        }else{
            perror("Errore apertura cartella file");
            exit(-1);
        }
    }else{
        struct dirent * elem = NULL;
        errno = 0;
        char * tmpPath = NULL;
        while((elem = readdir(fileDir)) != NULL){
            if (!strcmp(elem->d_name, ".") || !strcmp(elem->d_name, "..")) { // Ignoro il . e il ..
                continue;
            }
            tmpPath = malloc(sizeof(char) * (lenPath + 1 + strlen(elem->d_name) + 1)); // "path/elem->d_name'\0'"
            tmpPath[0] = '\0';
            strcat(tmpPath, configurazione->DirName);
            strcat(tmpPath, "/"); // TODO controllare errori
            strcat(tmpPath, elem->d_name); // TODO controllare errori
            remove(tmpPath); // TODO controllare errori
            free(tmpPath);
        }
        closedir(fileDir); // TODO controllare errore
        if(errno != 0){
            perror("Errore lettura file nella cartella");
        }
    }
}

void initConnessi(){
    utentiConnessi = malloc(sizeof(array_s));
    utentiConnessi->arr = malloc(sizeof(utente_connesso_s) * configurazione->MaxConnections);
    pthread_mutex_init(&(utentiConnessi->arr_m), NULL);
    utentiConnessi->nConnessi = 0;
    int i;
    for(i=0; i<configurazione->MaxConnections; i++){
        utentiConnessi->arr[i].nickname = NULL;
        utentiConnessi->arr[i].fd = -1;
        pthread_mutex_init(&(utentiConnessi->arr[i].fd_m), NULL);
    }
}

void initRegistrati(){
    utentiRegistrati = malloc(sizeof(hash_s));
    ec_null(utentiRegistrati->hash = icl_hash_create(HASHSIZE, NULL, compareString), "Errore creazione hash");
    initHashLock();
}

void initNSock(){
    nSock = malloc(sizeof(connessi_s));
    nSock->contatore = 0;
    pthread_mutex_init(&(nSock->contatore_m), NULL);
}

void initHashLock(){
    int i=0;
    for(i=0; i<(HASHSIZE / HASHGROUPSIZE); i++){
        pthread_mutex_init(&(utentiRegistrati->hash_m[i]), NULL);
    }
}

void stopPool(){
    int i;
    for(i=0; i<configurazione->ThreadsInPool; i++){
        int * toPush = malloc(sizeof(int));
        *toPush = -1;
        push(richieste, toPush); // TODO controllare errori
    }
}

static void * pool(void * arg){
    int * fd;
    int pipe = *((int *)arg);
    int ris, r;
    message_t msg;
    while(1){
        fd = (int *)pop(richieste);
        if(*fd == -1){
            free(fd);
            pthread_exit((void *)0);
        }
        r = readMsg(*fd, &msg);
        if(r < 0){ //TODO controllare errori.
            perror("Errore readMsg");
            //exit(-1);
        }else if(r == 0){
            // vuol dire che il client ha finito di comunicare, allora devo chiudere la connessione e eliminare l'utente dagli utenti connessi
            // è come se mi mandasse un messaggio con operazione DISCONNECT_OP
            // oss: Devo controllare se esiste nella lista degli utenti un utente_connesso_s->fd = *fd

            disconnect_op(*fd); // rimuovo l'utente dalla lista degli utenti connessi (se esisite)
            close(*fd); // Chiudo il socket. TODO controllare errore
            LOCKnSock();
            nSock->contatore --;
            UNLOCKnSock();
            // Quindi non ho letto nulla e non devo liberare niente
        }else{
            printf("Ricevuta op: %d da: %s con fd: %d\n", msg.hdr.op, msg.hdr.sender, *fd);
            //printf("--- MESSAGGIO ---\n");
            //printf("BUF: %s\nLEN: %d\nSENDER:%s\nRECEIVER:%s\n", msg.data.buf, msg.data.hdr.len, msg.hdr.sender, msg.data.hdr.receiver);

            switch(msg.hdr.op){
                case REGISTER_OP:
                    ris = register_op(*fd, msg); //TODO controllare errori
                    //printRisOP(msg, ris);
                    break;
                case CONNECT_OP:
                    ris = connect_op(*fd, msg, 0);
                    //printRisOP(msg, ris);
                    break;
                case POSTTXT_OP:
                    ris = posttxt_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                case POSTTXTALL_OP:
                    ris = posttxtall_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                case POSTFILE_OP:
                    ris = postfile_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                case GETFILE_OP:
                    ris = getfile_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                case GETPREVMSGS_OP:
                    ris = getprevmsgs_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                case USRLIST_OP:
                    ris = usrlist_op(*fd, msg, 0);
                    //printRisOP(msg, ris);
                    break;
                case UNREGISTER_OP:
                    ris = unregister_op(*fd, msg);
                    //printRisOP(msg, ris);
                    break;
                default: printf("Errore default\n");
                    //TODO gestione errore
            }
            if(writen(pipe, fd, sizeof(int)) == -1){
                perror("Errore writen");
                exit(-1);//TODO controlla errori
            }
            //printf("LENNNN: %d\n", msg.data.hdr.len);
            if(msg.data.hdr.len > 0){
                //printf("Eseguo free\n");
                free(msg.data.buf);
            }
        }


        free(fd);
    }
}

void printRisOP(message_t m, int ok){
    switch(m.hdr.op){
        case REGISTER_OP:
            if(ok == 0) printf("REGISTER_OP OK: %s\n", m.hdr.sender);
            else printf("REGISTER_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case CONNECT_OP:
            if(ok == 0) printf("CONNECT_OP OK: %s\n", m.hdr.sender);
            else printf("CONNECT_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case USRLIST_OP:
            if(ok == 0) printf("USRLIST_OP OK: %s\n", m.hdr.sender);
            else printf("USRLIST_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case UNREGISTER_OP:
            if(ok == 0) printf("UNREGISTER_OP OK: %s\n", m.hdr.sender);
            else printf("UNREGISTER_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case POSTTXT_OP:
            if(ok == 0) printf("POSTTXT_OP OK: %s\n", m.hdr.sender);
            else printf("POSTTXT_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case GETPREVMSGS_OP:
            if(ok == 0) printf("GETPREVMSGS_OP OK: %s\n", m.hdr.sender);
            else printf("GETPREVMSGS_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case POSTTXTALL_OP:
            if(ok == 0) printf("POSTTXTALL_OP OK: %s\n", m.hdr.sender);
            else printf("POSTTXTALL_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case GETFILE_OP:
            if(ok == 0) printf("GETFILE_OP OK: %s\n", m.hdr.sender);
            else printf("GETFILE_OP ERRORE: %s\n", m.hdr.sender);
            break;
        case POSTFILE_OP:
            if(ok == 0) printf("POSTFILE_OP OK: %s\n", m.hdr.sender);
            else printf("POSTFILE_OP ERRORE: %s\n", m.hdr.sender);
            break;
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
    for(i=0; i<configurazione->ThreadsInPool; i++){
        FD_SET(pfds[i][0], &set);
        if(pfds[i][0] > fd_num) fd_num = pfds[i][0];
    }
    FD_SET(fd_skt,&set);
    if(fd_skt > fd_num) fd_num = fd_skt;
    while (1) {
        rdset = set;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if (select(fd_num+1,&rdset,NULL,NULL,NULL) == -1) {
            perror("Errore select");
            exit(-1); // TODO gestione errore
        } else {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            int fd;
            for (fd = 0; fd<=fd_num;fd++) {
                if (FD_ISSET(fd, &rdset)) {
                    if (fd == fd_skt) { /* sock connect pronto */
                        int fd_c = accept(fd_skt, NULL, 0);
                        pthread_mutex_lock(&(nSock->contatore_m)); // TODO usare la funzione in struttureCondivise
                        if(nSock->contatore >= configurazione->MaxConnections){
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
                            printf("Inserito fd: %d\n", fd);
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

static void * segnali(void * arg){
    sigset_t toHandle = *((sigset_t *)arg);
    int sigRicevuto;

    while(sigwait(&toHandle, &sigRicevuto) == 0){ // non ci sono errori
        printf("Ricevuto segnale: %d\n", sigRicevuto);
        if(sigRicevuto == SIGUSR1){
            if(strlen(configurazione->StatFileName) > 0){
                FILE * fileStat;
                if((fileStat = fopen(configurazione->StatFileName, "a")) == NULL){ // "a" nella fopen corrisponde a O_WRONLY | O_CREAT | O_APPEND nella open
                    perror("Errore apertura file stat");
                    exit(-1);
                }
                LOCKStat();
                printStats(fileStat, statistiche->stat);
                UNLOCKStat();
                fclose(fileStat);
            }
        }else{ // Ho ricevuto o SIGQUIT o SIGTERM o SIGINT
            printf("LIBERO TUTTO\n");
            stopAllThread();
            pthread_exit((void *)0);
        }
    }
}

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

void stopAllThread(){
    // Eliminare tutti i thread
    pthread_cancel(allThread[1]); // Fermo subito il listener almeno non possono più arrivare le richieste
    stopPool(); // Fermo tutti i thread del pool
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
    if(remove(configurazione->UnixPath) == -1){
        if(errno != ENOENT) {// Se errno == ENOENT vuol dire che non esiste un file con quel path
            return -1;
        }
    }
    int fd_skt;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, configurazione->UnixPath, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt = socket(AF_UNIX,SOCK_STREAM,0)) == -1) return -1;
    ec_meno1_return(bind(fd_skt,(struct sockaddr *)&sa, sizeof(sa)), "Errore bind");
    ec_meno1_return(listen(fd_skt, SOMAXCONN), "Errore listen");
    return fd_skt;
}
