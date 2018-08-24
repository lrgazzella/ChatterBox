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
#include "./chatty.h"

// OSS: Nelle funzioni di init non controllo che la pthread_mutex_destroy vada a buon fine perchè può fallire solo per due motivi:
//       EBUSY -> sto facendo la destroy di una lock presa -> visto che sto inizializzando nessuno può prenderla
//       EINVAL -> la mutex passata alla destroy non è valida -> se non fosse valida mi sarei fermato prima

// OSS: Avvio prima il pool e poi il listener in modo che appena iniziano a inviarmi i messaggio ho tutti il pool pronto e non devo aspettare la creazione

/* Variabili globali */
Queue_t * richieste;
config * configurazione;
hash_s * utentiRegistrati;
array_s * utentiConnessi;
connessi_s * nSock;
stat_s * statistiche;
pthread_t * allThread;
int ** pfds;


int main(int argc, char *argv[]) {
    /* Prendo il file di configurazione */
    int optc;
    const char optstring[] = "f:";
    char * pathFileConf = NULL;
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
    ec_null(configurazione = malloc(sizeof(config)), "Errore. Spazio in memoria insufficiente");
    ec_null_c(initParseCheck(pathFileConf, configurazione), "Errore parsing file", free(configurazione));
    atexit(cleanupConfigurazione);
    ec_meno1(initDirFile(), "Errore inizializzazione directory file");
    ec_null(richieste = initQueue(), "Errore inizializzazione coda richieste");
    atexit(cleanupRichieste);
    ec_meno1(initStat(), "Errore inizializzazione statistiche");
    atexit(cleanupStat);
    ec_meno1(initNSock(), "Errore inizializzazione contatore socket attive");
    atexit(cleanupNSock);
    ec_meno1(initRegistrati(), "Errore inizializzazione hash registrati");
    atexit(cleanupRegistrati);
    ec_meno1(initConnessi(), "Errore inizializzazione array connessi");
    atexit(cleanupConnessi);


    /* Creazione pipe */
    ec_null(pfds = malloc(sizeof(int *) * configurazione->ThreadsInPool), "Errore. Spazio in memoria insufficiente");
    for(i=0; i<configurazione->ThreadsInPool; i++){
        if((pfds[i] = malloc(sizeof(int) * 2)) == NULL){
            int j;
            for(j=0; j<i; j++){
                free(pfds[j]);
            }
            free(pfds);
            perror("Errore. Spazio in memoria insufficiente");
            exit(EXIT_FAILURE);
        }
        if(pipe(pfds[i]) == -1){
            int j;
            for(j=0; j<i; j++){
                free(pfds[j]);
            }
            free(pfds);
            perror("Errore creazione pipe");
            exit(EXIT_FAILURE);
        }
    }
    atexit(cleanupPipe);

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
    ec_null(allThread = malloc(sizeof(pthread_t) * (configurazione->ThreadsInPool + 2)), "Errore. Spazio in memoria insufficiente"); // pool + listener + segnali
    atexit(cleanupThreadId);
    if(pthread_create(&(allThread[0]), NULL, &segnali, (void *)&toHandle) != 0){
            perror("Errore creazione thread segnali");
            exit(EXIT_FAILURE);
    }

    /* Avvio pool */
    for(i=0; i<configurazione->ThreadsInPool; i++){
        if(pthread_create(&(allThread[i+2]), NULL, &pool, &(pfds[i][1])) != 0){
            stopAllThread(1, 0, i);
            perror("Errore creazione thread pool");
            exit(EXIT_FAILURE);
        }
    }

    /* Avvio listener */
    if(pthread_create(&(allThread[1]), NULL, &listener, pfds) != 0){
        stopAllThread(1, 0, configurazione->ThreadsInPool);
        perror("Errore creazione thread listener");
        exit(EXIT_FAILURE);
    }

    printf(" -- Server avviato --\n");

    /* JOIN */
    joinAllThread();
    unlink(configurazione->UnixPath);
    return 0;
}

void cleanupConfigurazione(){
    printf("Cleanup configurazione\n");
    FreeConfig(configurazione);
}

void cleanupRichieste(){
    printf("Cleanup richieste\n");
    deleteQueue(richieste);
}

void cleanupStat(){
    printf("Cleanup stat\n");
    pthread_mutex_destroy(&(statistiche->stat_m));
    free(statistiche);
}

void cleanupNSock(){
    printf("Cleanup NSock\n");
    pthread_mutex_destroy(&(nSock->contatore_m));
    free(nSock);
}

void cleanupRegistrati(){
    printf("Cleanup registrati\n");
    int i;
    icl_hash_destroy(utentiRegistrati->hash, free, freeCoda);
    for(i=0; i<HASHSIZE / HASHGROUPSIZE; i++){
        pthread_mutex_destroy(&(utentiRegistrati->hash_m[i]));
    }
    free(utentiRegistrati);
}

void cleanupConnessi(){
    printf("Cleanup connessi\n");
    int i;
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname)
            free(utentiConnessi->arr[i].nickname);
        pthread_mutex_destroy(&(utentiConnessi->arr[i].fd_m));
    }
    free(utentiConnessi->arr);
    pthread_mutex_destroy(&(utentiConnessi->arr_m));
    free(utentiConnessi);
}

void cleanupPipe(){
    printf("Cleanup pipe\n");
    int i;
    for(i=0; i<configurazione->ThreadsInPool; i++){
        free(pfds[i]);
    }
    free(pfds);
}

void cleanupThreadId(){
    printf("Cleanup Thread Id\n");
    free(allThread);
}

void joinAllThread(){
    int i;
    pthread_join(allThread[0], NULL);
    pthread_join(allThread[1], NULL);
    for(i=0; i<configurazione->ThreadsInPool; i++){
        pthread_join(allThread[i+2], NULL);
    }
}

int initStat(){
    if((statistiche = malloc(sizeof(stat_s))) == NULL) return -1;
    statistiche->stat.nusers = 0;
    statistiche->stat.nonline = 0;
    statistiche->stat.ndelivered = 0;
    statistiche->stat.nnotdelivered = 0;
    statistiche->stat.nfiledelivered = 0;
    statistiche->stat.nfilenotdelivered = 0;
    statistiche->stat.nerrors = 0;
    if(pthread_mutex_init(&(statistiche->stat_m), NULL) != 0){
        free(statistiche);
        return -1;
    }
    return 0;
}

// Se esiste già la cartella con quel nome la svuota, altrimenti la crea
int initDirFile(){
    DIR * fileDir = NULL;
    size_t lenPath = strlen(configurazione->DirName);

    if((fileDir = opendir(configurazione->DirName)) == NULL){
        if(errno == ENOENT){ // la directory che ho cercato di aprire non esiste
            if(mkdir(configurazione->DirName, 0777) == -1){ // Errore creazione cartella
                return -1;
            }
        }else{ // Errore apertura cartella
            return -1;
        }
    }else{
        struct dirent * elem = NULL;
        errno = 0;
        char * tmpPath = NULL;
        while((elem = readdir(fileDir)) != NULL){
            if(errno != 0) return -1;
            if (!strcmp(elem->d_name, ".") || !strcmp(elem->d_name, "..")) { // Ignoro il . e il ..
                continue;
            }
            if((tmpPath = malloc(sizeof(char) * (lenPath + 1 + strlen(elem->d_name) + 1))) == NULL){ // "path/elem->d_name'\0'"
                return -1;
            }
            tmpPath[0] = '\0';
            strcat(tmpPath, configurazione->DirName);
            strcat(tmpPath, "/");
            strcat(tmpPath, elem->d_name);
            if(remove(tmpPath) == -1){
                free(tmpPath);
                closedir(fileDir);
                return -1;
            }
            free(tmpPath);
            errno = 0;
        }
        closedir(fileDir);
    }
    return 0;
}

int initConnessi(){
    if((utentiConnessi = malloc(sizeof(array_s))) == NULL) return -1;
    if((utentiConnessi->arr = malloc(sizeof(utente_connesso_s) * configurazione->MaxConnections)) == NULL){
        free(utentiConnessi);
        return -1;
    }
    if(pthread_mutex_init(&(utentiConnessi->arr_m), NULL) != 0){
        free(utentiConnessi->arr);
        free(utentiConnessi);
        return -1;
    }
    utentiConnessi->nConnessi = 0;
    int i;
    for(i=0; i<configurazione->MaxConnections; i++){
        utentiConnessi->arr[i].nickname = NULL;
        utentiConnessi->arr[i].fd = -1;
        if(pthread_mutex_init(&(utentiConnessi->arr[i].fd_m), NULL) != 0){
            int j;
            for(j=0; j<i; j++){
                pthread_mutex_destroy(&(utentiConnessi->arr[j].fd_m)); // Distruggo tutte le lock già create.
            }
            free(utentiConnessi->arr);
            free(utentiConnessi);
            return -1;
        }
    }
    return 0;
}

int initRegistrati(){
    if((utentiRegistrati = malloc(sizeof(hash_s))) == NULL) return -1;
    if((utentiRegistrati->hash = icl_hash_create(HASHSIZE, NULL, compareString)) == NULL){
        free(utentiRegistrati);
        return -1;
    }
    if(initHashLock() == -1){
        icl_hash_destroy(utentiRegistrati->hash, free, freeCoda);
        free(utentiRegistrati);
        return -1;
    }
    return 0;
}

int initNSock(){
    if((nSock = malloc(sizeof(connessi_s))) == NULL) return -1;
    nSock->contatore = 0;
    if(pthread_mutex_init(&(nSock->contatore_m), NULL) != 0){
        free(nSock);
        return -1;
    }
    return 0;
}

int initHashLock(){
    int i=0;
    for(i=0; i<(HASHSIZE / HASHGROUPSIZE); i++){
        if(pthread_mutex_init(&(utentiRegistrati->hash_m[i]), NULL) != 0){
            int j;
            for(j=0; j<i; j++){
                pthread_mutex_destroy(&(utentiRegistrati->hash_m[j]));
            }
            return -1;
        }
    }
    return 0;
}

void stopPool(int nThreadAttivi){
    int i;
    for(i=0; i<nThreadAttivi; i++){
        int * toPush;
        if((toPush = malloc(sizeof(int))) == NULL) exit(-1); // Non termino i thread visto che per terminarli dovrei richiamare questa funzione. Ma visto che non è riuscita ad allocare memoria questa volta, molto probabilmente non ci riuscirà nemmeno alla prossima
        *toPush = -1;
        push(richieste, toPush); // Non genera error
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
        if(r < 0){ // Non termino il server. Potrebbe essere che stavo leggendo mentre il client ha chiuso la socket. Non posso terminare l'intero server per un client che ha chiuso la socket
            printf("Errore lettura messaggio");
        }else if(r == 0){
            // vuol dire che il client ha finito di comunicare, allora devo chiudere la connessione e eliminare l'utente dagli utenti connessi
            // è come se mi mandasse un messaggio con operazione DISCONNECT_OP
            // oss: Devo controllare se esiste nella lista degli utenti un utente_connesso_s->fd = *fd

            disconnect_op(*fd); // rimuovo l'utente dalla lista degli utenti connessi (se esisite)
            close(*fd); // Chiudo la socket. Non controllo errori dato che qualli che ci sono non si possono verificare
            LOCKnSock();
            nSock->contatore --;
            UNLOCKnSock();
            // Quindi non ho letto nulla e non devo liberare niente
        }else{
            printf("Ricevuta op: %d da: %s con fd: %d\n", msg.hdr.op, msg.hdr.sender, *fd);
            switch(msg.hdr.op){
                case REGISTER_OP:
                    ris = register_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case CONNECT_OP:
                    ris = connect_op(*fd, msg, 0);
                    printRisOP(msg, ris);
                    break;
                case POSTTXT_OP:
                    ris = posttxt_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case POSTTXTALL_OP:
                    ris = posttxtall_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case POSTFILE_OP:
                    ris = postfile_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case GETFILE_OP:
                    ris = getfile_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case GETPREVMSGS_OP:
                    ris = getprevmsgs_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                case USRLIST_OP:
                    ris = usrlist_op(*fd, msg, 0);
                    printRisOP(msg, ris);
                    break;
                case UNREGISTER_OP:
                    ris = unregister_op(*fd, msg);
                    printRisOP(msg, ris);
                    break;
                default: printf("Ricevuto messaggio non valido\n");
            }
            if(writen(pipe, fd, sizeof(int)) == -1){
                perror("Errore comunicazione fd al listener");
                stopAllThread(1, 1, configurazione->ThreadsInPool - 1); // Se sono arrivato a questo punto vuol dire che c'è stata una richiesta -> th listener è attivo -> fermo anche lui
                pthread_exit((void *)-1);
            }
            if(msg.data.hdr.len > 0)
                free(msg.data.buf);
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
        stopAllThread(1, 0, configurazione->ThreadsInPool);
        pthread_exit((void *)-1);
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
            stopAllThread(1, 0, configurazione->ThreadsInPool);
            pthread_exit((void *)-1);
        } else {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            int fd;
            for (fd = 0; fd<=fd_num;fd++) {
                if (FD_ISSET(fd, &rdset)) {
                    if (fd == fd_skt) { /* sock connect pronto */
                        int fd_c = accept(fd_skt, NULL, 0);
                        LOCKnSock();
                        if(nSock->contatore >= configurazione->MaxConnections){
                            UNLOCKnSock();
                            message_t m;
                            setHeader(&(m.hdr), OP_FAIL, "");
                            setData(&(m.data), "", "Troppi utenti collegati", 24);
                            sendRequest(fd_c, &m);
                            close(fd_c);
                        } else {
                            nSock->contatore ++;
                            UNLOCKnSock();
                            FD_SET(fd_c, &set);
                            if (fd_c > fd_num) fd_num = fd_c;
                        }
                    } else {
                        int p;
                        if((p = isPipe(fd)) == 1){ // un thread del pool mi sta comunicando che un fd è di nuovo disponibile
                            int fd_c;
                            if(readn(fd, &fd_c, sizeof(int)) == -1){
                                perror("Errore lettura fd da riaggiungere");
                                stopAllThread(1, 0, configurazione->ThreadsInPool);
                                pthread_exit((void *)-1);
                            }
                            FD_SET(fd_c, &set);
                            if (fd_c > fd_num) fd_num = fd_c;
                        }else if(p == 0){ // un client mi sta mandando una richiesta
                            int * toPush;
                            if((toPush = malloc(sizeof(int))) == NULL){
                                perror("Errore. Spazio in memoria insufficiente");
                                stopAllThread(1, 0, configurazione->ThreadsInPool);
                                pthread_exit((void *)-1);
                            }
                            *toPush = fd;
                            push(richieste, toPush);
                            printf("Inserito fd: %d\n", fd);
                            FD_CLR(fd, &set);
                            fd_num = aggiorna(&set, fd_num);
                        }else{
                            perror("Errore stat nel fd da riaggiungere");
                            stopAllThread(1, 0, configurazione->ThreadsInPool);
                            pthread_exit((void *)-1);
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
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
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
            stopAllThread(0, 1, configurazione->ThreadsInPool);
            pthread_exit((void *)0);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
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

void stopAllThread(int segnali, int listener, int nThreadAttivi){ // Stoppa il listener e il pool. Se segnali = 1, stoppa anche il th dei segnali
    // Eliminare tutti i thread
    if(segnali) pthread_cancel(allThread[1]);
    if(listener) pthread_cancel(allThread[1]); // Fermo subito il listener almeno non possono più arrivare le richieste
    stopPool(nThreadAttivi); // Fermo tutti i thread del pool
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
    if(bind(fd_skt,(struct sockaddr *)&sa, sizeof(sa)) == -1) return -1;
    if(listen(fd_skt, SOMAXCONN) == -1) return -1;
    return fd_skt;
}
