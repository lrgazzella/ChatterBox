/** @file gestioneRichieste.c
  * @author Lorenzo Gazzella 546890
  * @brief Contiene l'implementazione di tutti i metodi per la gestione delle singole operazioni che il server gestisce.
  *        Constiene inoltre alcune funzioni di utilità utilizzate nel corpo delle funzione per gestire le richieste.
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
#include "gestioneRichieste.h"


/* Funzioni usate internamente */
char * makeListUsr();
int registrato(char * nickname);
message_t * copyMessage(message_t m);
int getUsrNickname(char * nickname);
int getUsrFd(long fd);
char * getOnlyFileName(char * path);
int getPosizioneLibera();


int register_op(long fd, message_t m){
    void * nickname = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1));
    strcpy((char *) nickname, m.hdr.sender);

    message_t r;
    if(strlen(m.hdr.sender) > MAX_NAME_LENGTH){ // Se la lunghezza del sender è maggiore di MAX_NAME_LENGTH -> sono sicuro che non è tra i registrati -> non è tra i connessi -> non prendo nessuna lock
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }
    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){ // Non esiste un utente con quel nickname, allora lo creo
        coda_circolare_s * hist = initCodaCircolare(configurazione->MaxMsgSize, freeMessage_t);

        if(icl_hash_insert(utentiRegistrati->hash, nickname, (void *)hist) == NULL){
            // Inserimento non andato a buon fine, allora rimuovo la coda che avevo creato per il nuovo utente
            UNLOCKRegistrati(m.hdr.sender);
            eliminaCoda(hist);
            free(nickname);
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }else{
            // Non rilascio la lock sui registrati perchè altrimenti qualcuno potrebbe vederlo e mandargli un messaggio. Se però la connessione dovesse fallire, dovrei azzerare l'intera operazione
            if(connect_op(fd, m, 1) == -1){ // La connect non è andata a buon fine, allora devo annullare la registrazione
                icl_hash_delete(utentiRegistrati->hash, nickname, free, freeCoda); // Mi libera direttamente anche la history
                UNLOCKRegistrati(m.hdr.sender);
                // free(nickname); non serve, la free del nickname la fa icl_hash_delete
                return -1;
            }else {
                UNLOCKRegistrati(m.hdr.sender);
                ADDStat("nusers", 1);
                return 0;
            }
        }
    }else{ //Esiste già un utente con quel nickname, mando un messaggio di errore OP_NICK_ALREADY
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_ALREADY, "");
        sendHeader(fd, &(r.hdr));
        free(nickname);
        ADDStat("nerrors", 1);
        return -1;
    }
}

int connect_op(long fd, message_t m, int atomica){
    message_t r;
    if(!atomica){
        LOCKRegistrati(m.hdr.sender);
        if(!registrato(m.hdr.sender)){ // Non esiste un utente registrato con questo nickname, restituisco un messaggio di errore OP_NICK_UNKNOWN
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr)); // rispondo direttamente sull'fd con cui mi ha fatto la richiesta
            ADDStat("nerrors", 1);
            return -1;
        }
    } // se è atomica vuol dire che la connect_op è stata chiamata dalla register_op. Quindi sono sicuro che l'utente è registrato

    // Esiste un utente registrato con questo nickname, allora lo aggiungo alla lista degli utenti connessi
    LOCKConnessi();
    int pos;
    if((pos = getUsrNickname(m.hdr.sender)) != -1){ // Controllo se è già connesso
        // Esisite un utente già connesso con lo stesso nickname
        if(!atomica) UNLOCKRegistrati(m.hdr.sender);
        UNLOCKConnessi();
        setHeader(&(r.hdr), OP_FAIL, ""); // Se non facessi questo controllo potrei dire, a un utente che ha sbagliato l'inserimento del nickname, che la connessione è andata a buon fine, mentre il nickname inserito è già connesso
        sendHeader(fd, &(r.hdr)); // rispondo sull'fd. Questo fd è quello con cui mi hanno fatto la richiesta, non quello dell'utente connesso
        ADDStat("nerrors", 1);
        return -1;
    }
    // Ora sono certo che il nickname è registrato ma non connesso -> lo connetto
    /* Creazione nuovo utente */
    int nuovo = getPosizioneLibera();
    LOCKfd(nuovo); // non rilascio mai la lock sui connessi -> una volta presa la lock non serve guardare se la posizione è ancora libera
    utentiConnessi->arr[nuovo].nickname = strndup(m.hdr.sender, (MAX_NAME_LENGTH + 1));
    utentiConnessi->arr[nuovo].fd = fd;
    utentiConnessi->nConnessi ++;

    if(usrlist_op(fd, m, 1) == -1){ // usrlist_op fallita, allora elimino nuovoUtente dall'array
        utentiConnessi->nConnessi --;
        utentiConnessi->arr[nuovo].fd = -1;
        free(utentiConnessi->arr[nuovo].nickname);
        utentiConnessi->arr[nuovo].nickname = NULL;
        UNLOCKfd(nuovo);
        UNLOCKConnessi();
        if(!atomica) UNLOCKRegistrati(m.hdr.sender);
        return -1;
    }else{
        UNLOCKfd(nuovo);
        UNLOCKConnessi();
        if(!atomica) UNLOCKRegistrati(m.hdr.sender);
        ADDStat("nonline", 1);
        return 0;
    }

}

int usrlist_op(long fd, message_t m, int atomica){
    message_t r;
    if(!atomica) {
        LOCKRegistrati(m.hdr.sender);
        if(!registrato(m.hdr.sender)){ // Controllo se l'utente è registrato. Se non è registrato gli mando un errore OP_NICK_UNKNOWN
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }
        int pos;
        LOCKConnessi();
        if((pos = getUsrNickname(m.hdr.sender)) != -1){ // Controllo se l'utente è connesso. Se è connesso bene, altrimenti ritorno errore
            // Esisite un utente già connesso con lo stesso nickname
            UNLOCKRegistrati(m.hdr.sender);
            char * listStr = makeListUsr();
            if(listStr == NULL){ // Errore -> fermo il server
                UNLOCKConnessi();
                perror("Errore creazione file");
                stopAllThread(1, 1, configurazione->ThreadsInPool);
                return -1;
            }
            int nUsr = utentiConnessi->nConnessi;
            LOCKfd(pos);
            UNLOCKConnessi();

            // Devo farla sotto la makeListUsr. L'alternativa sarebbe stata quella di mettere la unlock prima dell'if che controlla se il nickname ottenuto dopo la lock sull'fd è uguale a quello che sto analizzando e di fare la lock dei connessi nella funzione makeListUsr. Avrei però rischiato di andare in deadlock. Io a questo punto ho la lock sull'fd. Se nel frattempo si inseriva una richiesta di unregister, prendeva la lock sui connessi e si metteva in attesa per la lock sull'fd. Ora quando la makeListUsr andava a fare la lock sui connessi restava in attesa dato che al momento è della funzione unregister. Ma la unregister non può andare avanti finchè non termina la usrlist, ovvero fino a quando non termina la makeListUsr.
            setHeader(&(r.hdr), OP_OK, "");
            setData(&(r.data), "", listStr, (MAX_NAME_LENGTH + 1) * nUsr);
            sendRequest(fd, &r);
            UNLOCKfd(pos);
            free(r.data.buf); //r.data.buf non potrà mai essere vuoto -> almeno l'utente che sta chiamando la usrlist_op è connesso
            // free(listStr); liberando r.data.buf libero anche listStr
            return 0;
        }else{ // utente non connesso -> non devo prendere la lock su alcun fd
            UNLOCKRegistrati(m.hdr.sender);
            UNLOCKConnessi();
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }
    }else{
        char * listStr = makeListUsr(); // ho la lock sui registrati, sui connessi e sull'fd
        int nUsr = utentiConnessi->nConnessi;
        setHeader(&(r.hdr), OP_OK, "");
        setData(&(r.data), "", listStr, (MAX_NAME_LENGTH + 1) * nUsr);
        sendRequest(fd, &r);
        // free(r.data.buf); -> r.data.buf e listStr puntano alla stessa area di memoria
        free(listStr);
        return 0;
    }
}

int unregister_op(long fd, message_t m){
    message_t r;
    int pos;

    if(strcmp(m.hdr.sender, m.data.hdr.receiver) != 0){ // se i due nickname sono diversi -> errore
        LOCKRegistrati(m.hdr.sender);
        if(!registrato(m.hdr.sender)) {
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }
        LOCKConnessi(); // Non posso rilasciare i connessi fino a quando non ho finito di gestire l'intera richiesta. Questo perchè se ci fosse una richiesta di usrlist_op, qeusta potrebbe prendere la lock sui connessi e restituire una lista dei connessi errata:

        if((pos = getUsrNickname(m.hdr.sender)) != -1) { // il sender è connesso -> prendo la lock e comunico l'errore
            LOCKfd(pos); // non ho mai rilasciato le lock sui registrati e sui connessi -> quando acquisisco la lock, non serve controllare se l'utente è rimasto quello.
            UNLOCKConnessi();
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            UNLOCKfd(pos);
            ADDStat("nerrors", 1);
            return -1;
        }else{
            UNLOCKConnessi();
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }
    }

    // I due nickname sono uguali
    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)) {
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    if(disconnect_op(fd) == -1) { // ha tentato di disconnetterlo ma non era connesso
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }
    icl_hash_delete(utentiRegistrati->hash, (void *)&(m.hdr.sender), free, freeCoda); // Mi libera direttamente anche la history. Non controllo errore dato che icl_hash_delete torna -1 solo nel caso in cui l'utente da eliminare non esista. Ma questo non può accadere altirmenti avrei mandato errore sopra

    UNLOCKRegistrati(m.hdr.sender);
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(fd, &(r.hdr));
    ADDStat("nusers", -1);
    return 0;
}

int disconnect_op(long fd){
    LOCKConnessi();
    int pos = getUsrFd(fd);
    if(pos == -1){ // nessun utente con questo fd connesso
        UNLOCKConnessi();
        return -1;
    }
    LOCKfd(pos);
    free(utentiConnessi->arr[pos].nickname);
    utentiConnessi->arr[pos].nickname = NULL;
    utentiConnessi->arr[pos].fd = -1;
    utentiConnessi->nConnessi --;
    UNLOCKfd(pos);
    UNLOCKConnessi();
    ADDStat("nonline", -1);
    return 0;
}

int posttxt_op(long fd, message_t m){
    message_t r;
    int posSender, posReceiver;

    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKConnessi();
    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKfd(posSender); // Non ho rilasciato la lock sui connessi
    UNLOCKRegistrati(m.hdr.sender);
    UNLOCKConnessi();

    if(strcmp(m.hdr.sender, m.data.hdr.receiver) == 0){ // Non può mandarsi messaggi da solo
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        return -1;
    }

    if(m.data.hdr.len > configurazione->MaxMsgSize){ // Controllo subito se la lunghezza del messaggio rispetta il file di configurazione-> se non rispetta -> errore
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        return -1;
    }
    UNLOCKfd(posSender);

    coda_circolare_s * h;
    LOCKRegistrati(m.data.hdr.receiver);
    if((h = icl_hash_find(utentiRegistrati->hash, (void *)&(m.data.hdr.receiver))) == NULL){
        UNLOCKRegistrati(m.data.hdr.receiver);
        LOCKfd(posSender);
        if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){ // è stato disconnesso
            UNLOCKfd(posSender);
            ADDStat("nerrors", 1);
            return -1;
        }
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        return -1;
    }

    message_t * toSend = copyMessage(m);
    toSend->hdr.op = TXT_MESSAGE;
    LOCKConnessi();
    if((posReceiver = getUsrNickname(m.data.hdr.receiver)) != -1){ // receiver connesso -> gli invio il messaggio
         LOCKfd(posReceiver); // Non rilascio la lock dei connnessi
         sendRequest(utentiConnessi->arr[posReceiver].fd, toSend);
         UNLOCKfd(posReceiver);
         ADDStat("ndelivered", 1);
    }else{
         ADDStat("nnotdelivered", 1); // Lo devo fare solo se l'utente non è online
    }
    UNLOCKConnessi();

    /* Aggiungo il messaggio alla history del receiver */
    inserisci(h, toSend); // toSend lo libererà la unregister_op
    UNLOCKRegistrati(m.data.hdr.receiver);
    LOCKfd(posSender);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){
        UNLOCKfd(posSender);
        return 0;
    }
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    return 0;
}

int getprevmsgs_op(long fd, message_t m){
    message_t r;
    coda_circolare_s * h;
    int pos;

    LOCKRegistrati(m.hdr.sender);
    if((h = (coda_circolare_s *)icl_hash_find(utentiRegistrati->hash, (void *)&(m.hdr.sender))) == NULL){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKConnessi();
    if((pos = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKfd(pos);
    UNLOCKConnessi();


    /* Comunico al client quanti messaggi sono contenuti nella history */
    size_t dim = (size_t)lung(h);
    setHeader(&(r.hdr), OP_OK, "");
    setData(&(r.data), "", (const char *) &dim, (unsigned int)sizeof(size_t*));
    sendRequest(utentiConnessi->arr[pos].fd, &r);

    /* Invio di ogni messaggio */
    coda_circolare_iteratore_s * it = initIteratore(h);
    message_t * toSend;
    if(it){ // Se la history è vuota initIteratore(h) torna NULL
        while((toSend = (message_t *)next(it)) != NULL){
            sendRequest(utentiConnessi->arr[pos].fd, toSend); // TODO controllare errori
        }
    }
    UNLOCKRegistrati(m.hdr.sender); // Visto che sto usando un elemento della hash, non posso rilasciarla prima la lock
    UNLOCKfd(pos);


    eliminaIteratore(it);
    return 0;
}

int posttxtall_op(long fd, message_t m){
    message_t r;
    int posSender;

    LOCKRegistrati(m.hdr.sender);

    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKConnessi();

    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKfd(posSender);
    UNLOCKRegistrati(m.hdr.sender);

    if(m.data.hdr.len > configurazione->MaxMsgSize){ // Controllo subito se la lunghezza del messaggio rispetta il file di configurazione-> se non rispetta -> errore
        UNLOCKConnessi();
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        return -1;
    }
    UNLOCKfd(posSender);
    UNLOCKConnessi();
    icl_entry_t * bucket;
    icl_entry_t * currUsr;
    int posReceiver;
    int i;

    m.hdr.op = TXT_MESSAGE; // è il messaggio che devo inviare a tutti gli altri client

    for (i=0; i<utentiRegistrati->hash->nbuckets; i++) {
        LOCKPosRegistrati(i);
        LOCKConnessi();
        bucket = utentiRegistrati->hash->buckets[i];
        for (currUsr=bucket; currUsr!=NULL; currUsr=currUsr->next) {
            if(strcmp(currUsr->key, m.hdr.sender) != 0){ // altimenti si invierebbe il messaggio da solo
                if((posReceiver = getUsrNickname(currUsr->key)) != -1){ // se è connesso gli invio il messaggio.
                    LOCKfd(posReceiver);
                    sendRequest(utentiConnessi->arr[posReceiver].fd, &m);
                    UNLOCKfd(posReceiver);
                    ADDStat("ndelivered", 1);
                }else{
                    ADDStat("nnotdelivered", 1);
                }
                // In ogni caso aggiungo il messaggio alla sua history
                inserisci((coda_circolare_s *)currUsr->data, copyMessage(m));
            }
        }
        UNLOCKConnessi();
        UNLOCKPosRegistrati(i);
    }
    LOCKfd(posSender);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){
        UNLOCKfd(posSender); // è stato disconnesso mentre inviavo i messaggi
        return 0;
    }
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    return 0;
}

int postfile_op(long fd, message_t m){// m.data.hdr.len tiene già conto dello '\0'
    message_t r, file;
    int posSender, posReceiver;

    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKConnessi();
    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }
    LOCKfd(posSender);
    UNLOCKConnessi();
    UNLOCKRegistrati(m.hdr.sender);

    /* Lettura file */
    char * fileName = getOnlyFileName(m.data.buf);

    char * path = calloc(sizeof(char), (strlen(configurazione->DirName) + 1 + strlen(fileName) + 1));
    path[0] = '\0';
    strcat(path, configurazione->DirName);
    strcat(path, "/");
    strcat(path, fileName);
    readData(fd, &(file.data)); // Leggo il file vero e proprio

    // file.data.hdr.len corrisponde alla dimensione del file in byte
    if((file.data.hdr.len / 1024) > configurazione->MaxFileSize){
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        free(path);
        free(file.data.buf);
        ADDStat("nerrors", 1);
        return -1;
    }
    UNLOCKfd(posSender);

    /* Controllo se il receiver è registrato */
    coda_circolare_s * h;
    LOCKRegistrati(m.data.hdr.receiver);
    if((h = (coda_circolare_s *)icl_hash_find(utentiRegistrati->hash, m.data.hdr.receiver)) == NULL){
        UNLOCKRegistrati(m.data.hdr.receiver);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        LOCKfd(posSender);
        if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){ // è stato disconnesso
            UNLOCKfd(posSender);
            ADDStat("nerrors", 1);
            return -1;
        }
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        return -1;
    }

    /* Qui sono sicuro che il receiver sia registrato e ho la relativa lock sulla hash */
    // OSS: Non posso rilasciar la lock sui registrati relativa al receiver, altrimenti potrebbero deregistrarlo

    int fdNewFile;
    if((fdNewFile = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1){
        UNLOCKRegistrati(m.data.hdr.receiver);
        free(file.data.buf);
        free(path);
        perror("Errore creazione file");
        stopAllThread(1, 1, configurazione->ThreadsInPool);
        return -1;
    }
    write(fdNewFile, file.data.buf, file.data.hdr.len);
    close(fdNewFile);
    free(file.data.buf);
    // File creato


    /* Lo comunico all'utente*/
    message_t toSend;
    setHeader(&(toSend.hdr), FILE_MESSAGE, m.hdr.sender);
    setData(&(toSend.data), "", strdup(fileName), strlen(fileName)); // m.data.buf contiene il path. Io invece devo comunicare il nome del file. Allora comunico fileName

    LOCKConnessi();
    if((posReceiver = getUsrNickname(m.data.hdr.receiver)) != -1){ // se è connesso gli mando subito un messaggio
        LOCKfd(posReceiver);
        sendRequest(utentiConnessi->arr[posReceiver].fd, &toSend);
        UNLOCKfd(posReceiver);
        ADDStat("nfiledelivered", 1);
    }else{
        ADDStat("nfilenotdelivered", 1);
    }
    UNLOCKConnessi();

    // In ogni caso lo aggiungo sulla sua history
    inserisci(h, copyMessage(toSend));
    free(toSend.data.buf);
    UNLOCKRegistrati(m.data.hdr.receiver);
    setHeader(&(r.hdr), OP_OK, "");
    LOCKfd(posSender);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){ // è stato disconnesso
        UNLOCKfd(posSender);
        free(path);
        return 0;
    }
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    free(path);
    return 0;
}

int getfile_op(long fd, message_t m){
    message_t r;
    int pos;

    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKConnessi();
    if((pos = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKRegistrati(m.hdr.sender);
        UNLOCKConnessi();
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        return -1;
    }

    LOCKfd(pos);
    UNLOCKRegistrati(m.hdr.sender);
    UNLOCKConnessi();

    char * path = malloc(sizeof(char) * (strlen(configurazione->DirName) + 1 + m.data.hdr.len + 1));
    path[0] = '\0';
    strcat(path, configurazione->DirName);
    strcat(path, "/");
    strcat(path, m.data.buf);

    if(access(path, F_OK) == -1) {
        setHeader(&(r.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(utentiConnessi->arr[pos].fd, &(r.hdr));
        UNLOCKfd(pos);
        free(path);
        ADDStat("nerrors", 1);
        return -1;
    }
    UNLOCKfd(pos);
    /* Ora non ho nessuna lock. Tanto il client ha inviato questa richiesta quando era online. Se di disconnette ora, porto comunque a termine questa richiesta */

    int fdToSend;
    if((fdToSend = open(path, O_RDONLY)) == -1){
        perror("Errore apertura file");
        free(path);
        stopAllThread(1, 1, configurazione->ThreadsInPool);
        return -1;
    }
    struct stat statBuf;
    if(fstat(fdToSend, &statBuf) == -1){
        perror("Errore stat file");
        free(path);
        stopAllThread(1, 1, configurazione->ThreadsInPool);
        return -1;
    }

    char * mappedfile = mmap(NULL, statBuf.st_size, PROT_READ,MAP_PRIVATE, fdToSend, 0);
    if(mappedfile == MAP_FAILED){
        perror("Errere mmap");
        stopAllThread(1, 1, configurazione->ThreadsInPool);
        return -1;
    }
    close(fdToSend);
    setHeader(&(r.hdr), OP_OK, "");
    setData(&(r.data), "", mappedfile, statBuf.st_size);
    LOCKfd(pos);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[pos].nickname) != 0 || utentiConnessi->arr[pos].fd != fd){ // è stato disconnesso nel frattempo
        UNLOCKfd(pos);
        return 0;
    }
    sendRequest(utentiConnessi->arr[pos].fd, &r);
    UNLOCKfd(pos);
    munmap(mappedfile, statBuf.st_size);
    free(path);
    return 0;
}




/**
  * @function getUsrNickname
  * @brief Funzione che restituisce la posizione di un nickname all'interno dell'array dei connessi
  *
  * @param nickname Nickname da cercare
  *
  * @return Se esiste un utente connesso con quel nickname, restituisce la sua posizone. -1 altrimenti
  */
int getUsrNickname(char * nickname){
    int i;
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname) // controllo se il nickname è null
            if(strcmp(utentiConnessi->arr[i].nickname, nickname) == 0)
                return i;
    }
    return -1;
}


/**
  * @function getUsrFd
  * @brief Funzione che restituisce la posizione di un fd all'interno dell'array dei connessi
  *
  * @param fd fd da cercare
  *
  * @return Se esiste un utente connesso con quel fd, restituisce la sua posizone. -1 altrimenti
  */
int getUsrFd(long fd){
    int i = 0;
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].fd == fd)
            return i;
    }
    return -1;
}


/**
  * @function getPosizioneLibera
  * @brief Funzione che restituisce prima posizione libera all'interno dell'array dei connessi
  *
  * @param fd fd da cercare
  *
  * @return La posizone in caso di successo. -1 altrimenti
  */
int getPosizioneLibera(){
    int i;
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname == NULL && utentiConnessi->arr[i].fd == -1){ // basterebbe anche solo una delle due condizioni
            return i;
        }
    }
    return -1; // Non potrà mai succedere visto che verrà bloccato prima dal listener
}
/**
  * @function copyMessage
  * @brief Funzione che esegue la copia di un messaggio
  *
  * @param m Messaggio da copiare
  *
  * @return Nuovo messaggio in caso di successo. NULL altrimenti
  */
message_t * copyMessage(message_t m){ // TODO controllare errore quando si chiama
    message_t * toAdd;
    if((toAdd = malloc(sizeof(message_t))) == NULL) return NULL;
    setHeader(&(toAdd->hdr), m.hdr.op, m.hdr.sender);
    setData(&(toAdd->data), m.data.hdr.receiver, strdup(m.data.buf), m.data.hdr.len);
    return toAdd;
}
/**
  * @function registrato
  * @brief Funzione boolena che controlla s eun determinato nickname è presente nella hash table dei registrati
  *
  * @param nickname Nickname da controllare
  *
  * @return 1 nel caso in cui sia registrato. 0 altrimenti
  */
int registrato(char * nickname){
    return icl_hash_find(utentiRegistrati->hash, (void *)nickname) != NULL;
}
/**
  * @function getOnlyFileName
  * @brief Funzione che dato un path, restituisce il nome del file associato al path
  *
  * @param path Path da controllare
  *
  * @return Nome del file associato al path
  */
char * getOnlyFileName(char * path){
    char * token, * last;
    last = token = strtok(path, "/");
    while((token = strtok(NULL, "/")) != NULL) last = token;
    return last;
}
/**
  * @function makeListUsr
  * @brief Funzione che restiruisce la lista degli utenti connessi
  *
  * @return Lista utenti connessi
  */
char * makeListUsr(){ // TODO controllare errore quando si chiama
    int i = 0;
    char * r;
    if((r = malloc(sizeof(char) * ((MAX_NAME_LENGTH + 1) * utentiConnessi->nConnessi))) == NULL) return NULL;
    if(!r) {
        printf("Errore calloc\n");
        exit(-1);
    }
    memset(r, 0, sizeof(char) * ((MAX_NAME_LENGTH + 1) * utentiConnessi->nConnessi));
    char * tmp = r;
    for(i=0; i<configurazione->MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname != NULL && utentiConnessi->arr[i].fd != -1){ // Sono le posizioni dell'array in cui non ci sono utenti
            strncpy(tmp, utentiConnessi->arr[i].nickname, MAX_NAME_LENGTH + 1); // basterebbe anche solo una delle due condizioni
            tmp += MAX_NAME_LENGTH + 1;
        }
    }
    return r;
}
