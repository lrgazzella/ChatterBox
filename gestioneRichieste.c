#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"
#include "./lib/GestioneHistory/codaCircolare.h"

void concat(char * str1, char * str2, int first);
char * makeListUsr();
int registrato(char * nickname);
message_t * copyMessage(message_t m);
void freeCoda(void * c);
char * intToString(int n);
int getUsrNickname(char * nickname);
int getUsrFd(long fd);
int getPosizioneLibera();
/* per tutte le funzioni: 0 successo, -1 errore */

int register_op(long fd, message_t m){
    void * nickname = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1));
    strcpy((char *) nickname, m.hdr.sender);
    
    message_t r;
    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){ // Non esiste un utente con quel nickname, allora lo creo
        coda_circolare_s * hist = initCodaCircolare(configurazione.MaxMsgSize, freeMessage_t);

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
            if(connect_op(fd, m, 1) == -1){ // La connect non è andata a buon fine, allora devo annullare la registrazione
                icl_hash_delete(utentiRegistrati->hash, nickname, free, freeCoda); // Mi libera direttamente anche la history
                UNLOCKRegistrati(m.hdr.sender);
                free(nickname);
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

// atomica == 1 -> ho già la lock sui registrati
int connect_op(long fd, message_t m, int atomica){
    // Se deve fare la lock della hash deve avere atomica = 0
    void * nickname = (void *)(m.hdr.sender);
    message_t r;
    if(!atomica){
        LOCKRegistrati(m.hdr.sender);
        if(!registrato((char *)nickname)){ // Non esiste un utente registrato con questo nickname, restituisco un messaggio di errore OP_NICK_UNKNOWN
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr)); // rispondo direttamente sull'fd con cui mi ha fatto la richiesta
            ADDStat("nerrors", 1);
            return -1;
        }
    } // se è atomica vuol dire che la connect_op è stata chiamata dalla register_op. Quindi sono sicuro che l'utente è registrato

    // Esiste un utente registrato con questo nickname, allora lo aggiungo alla lista degli utenti connessi
    /* Non posso rilasciare la lock della hash prima di aver aggiunto nuovoUtente alla lista degli utenti connessi. Altrimenti se client1 facesse una richiesta UNREGISTER_OP per clientCorrente, il client1 non vedrebbe che clientCorrente è dentro la lista degli utenti connessi, quindi non lo eliminerebbe e resterebbe per sempre nella lista */
    LOCKConnessi();
    int pos;
    if((pos = getUsrNickname(nickname)) != -1){ // Controllo se è già connesso
        // Esisite un utente già connesso con lo stesso nickname
        if(!atomica) UNLOCKRegistrati(m.hdr.sender); // TODO va bene rilasciare le lock prima di inviare il messaggio?
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
        if(!atomica) UNLOCKRegistrati(m.hdr.sender); // TODO non posso rilasciarla prima?
        return -1;
    }else{
        UNLOCKfd(nuovo);
        UNLOCKConnessi();
        if(!atomica) UNLOCKRegistrati(m.hdr.sender);
        ADDStat("nonline", 1);
        return 0;
    }

}

// atomica == 1 -> ho già la lock sui registrati, sui connessi e sull'fd su cui dovrò rispondere
int usrlist_op(long fd, message_t m, int atomica){
    void * nickname = (void *)(m.hdr.sender);
    message_t r;
    if(!atomica) {
        LOCKRegistrati(m.hdr.sender);
        if(!registrato((char *)nickname)){ // Controllo se l'utente è registrato. Se non è registrato gli mando un errore OP_NICK_UNKNOWN
            UNLOCKRegistrati(m.hdr.sender);
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr));
            ADDStat("nerrors", 1);
            return -1;
        }
        int pos;
        LOCKConnessi();
        if((pos = getUsrNickname(nickname)) != -1){ // Controllo se l'utente è connesso. Se è connesso bene, altrimenti ritorno errore
            // Esisite un utente già connesso con lo stesso nickname
            UNLOCKConnessi();
            UNLOCKRegistrati(m.hdr.sender);
            LOCKfd(pos);
            if(strcmp(m.hdr.sender, utentiConnessi->arr[pos].nickname) != 0 || utentiConnessi->arr[pos].fd != fd){
                // Vuol dire che nel frattempo che prendevo la lock, il client è stato disconnesso
                UNLOCKfd(pos);
                setHeader(&(r.hdr), OP_FAIL, "");
                sendHeader(fd, &(r.hdr));
                ADDStat("nerrors", 1);
                return -1;
            }
            LOCKConnessi();
            char * listStr = makeListUsr();
            int nUsr = utentiConnessi->nConnessi;
            UNLOCKConnessi(); // TODO controllare possibili deadlock
            // Devo farla sotto la makeListUsr. L'alternativa sarebbe stata quella di mettere la unlock prima dell'if che controlla se il nickname ottenuto dopo la lock sull'fd è uguale a quello che sto analizzando e di fare la lock dei connessi nella funzione makeListUsr. Avrei però rischiato di andare in deadlock. Io a questo punto ho la lock sull'fd. Se nel frattempo si inseriva una richiesta di unregister, prendeva la lock sui connessi e si metteva in attesa per la lock sull'fd. Ora quando la makeListUsr andava a fare la lock sui connessi restava in attesa dato che al momento è della funzione unregister. Ma la unregister non può andare avanti finchè non termina la usrlist, ovvero fino a quando non termina la makeListUsr.
            setHeader(&(r.hdr), OP_OK, "");
            setData(&(r.data), "", listStr, (MAX_NAME_LENGTH + 1) * nUsr);
            sendRequest(fd, &r);
            UNLOCKfd(pos);
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
        return 0;
    }
}

int unregister_op(long fd, message_t m){ // va chiamata con la lock sulla tabella hash (blocco del receiver e del sender) e sulla lista degli utenti connessi
    void * nickname = (void *)(m.hdr.sender);
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
            LOCKfd(pos); // non ho mai rilasciato le lock sui registrati e sui connessi -> quando acquisisco la lock, non serve controllare se l'utente è rimasto quello. TODO controllare questo ragionamento
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
    icl_hash_delete(utentiRegistrati->hash, nickname, free, freeCoda); // Mi libera direttamente anche la history. Non controllo errore dato che icl_hash_delete torna -1 solo nel caso in cui l'utente da eliminare non esista. Ma questo non può accadere altirmenti avrei mandato errore sopra

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
    LOCKfd(pos); // La disconnessione viene fatta dal client quando la read torna 0. Quindi non è possibile che mi disconnettano questo client quando rilascio la lock sui connessi
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
    ADDStat("nnotdelivered", 1);

    void * sender = (void *)(m.hdr.sender);
    void * receiver = (void *)(m.data.hdr.receiver);
    // TODO controllare che sender e receiver non siano uguali. Altrimenti avrei che un client si può mandare un messaggio da solo
    message_t r;
    int posSender, posReceiver;

    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -1);
        return -1;
    }

    LOCKConnessi();
    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -1);
        return -1;
    }

    UNLOCKConnessi();
    UNLOCKRegistrati(m.hdr.sender);

    LOCKfd(posSender);
    // TODO visto che prima era connesso potrei pure evitare questo controllo e inviare comunque il messaggio al receiver
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){ // il sender è stato deregistrato nel frattempo che era in attesa sulla lock
        UNLOCKfd(posSender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -1);
        return -1;
    }

    if(m.data.hdr.len > configurazione.MaxMsgSize){ // Controllo subito se la lunghezza del messaggio rispetta il file di configurazione. se non rispetta -> errore
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -1);
        return -1;
    }

    coda_circolare_s * h;
    LOCKRegistrati(m.data.hdr.receiver);
    if((h = icl_hash_find(utentiRegistrati->hash, (void *)receiver)) == NULL){
        UNLOCKRegistrati(m.data.hdr.receiver);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -1);
        return -1;
    }

    LOCKConnessi();
    if((posReceiver = getUsrNickname(m.data.hdr.receiver)) != -1){ // receiver connesso -> gli invio il messaggio
         int fdReceiver = utentiConnessi->arr[posReceiver].fd;
         UNLOCKConnessi();
         LOCKfd(posReceiver);
         if(strcmp(m.data.hdr.receiver, utentiConnessi->arr[posReceiver].nickname) == 0 || utentiConnessi->arr[posReceiver].fd == fdReceiver){
             m.hdr.op = TXT_MESSAGE;
             sendRequest(utentiConnessi->arr[posReceiver].fd, &m);
         }
         UNLOCKfd(posReceiver);
    }
    UNLOCKConnessi();

    /* Aggiungo il messaggio alla history del receiver */
    inserisci(h, copyMessage(m));
    UNLOCKRegistrati(m.data.hdr.receiver);
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    ADDStat("nnotdelivered", -1);
    ADDStat("ndelivered", 1);
    return 0;
}

int getprevmsgs_op(long fd, message_t m){
    void * sender = (void *)(m.hdr.sender);

    message_t r;
    coda_circolare_s * h;
    int pos;

    LOCKRegistrati(m.hdr.sender);
    if((h = (coda_circolare_s *)icl_hash_find(utentiRegistrati->hash, sender)) == NULL){
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
    UNLOCKConnessi();
    UNLOCKRegistrati(m.hdr.sender);
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
    ADDStat("nnotdelivered", utentiConnessi->nConnessi);
    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -utentiConnessi->nConnessi);
        return -1;
    }

    // UNLOCKConnessi(); Utenti connessi non posso rilasciarlo qui -> serve nei for per vedere se gli utenti sono connessi
    UNLOCKRegistrati(m.hdr.sender);

    LOCKfd(posSender);
    // TODO Se sono stato disconnesso mentre prendevo la lock, devo comunque inviare i messaggi visto che all'inizio ero connesso
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){ // Sono stato disconnesso mentre prendevo la lock
        UNLOCKfd(posSender);
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -utentiConnessi->nConnessi);
        return -1;
    }

    if(m.data.hdr.len > configurazione.MaxMsgSize){ // Controllo subito se la lunghezza del messaggio rispetta il file di configurazione. se non rispetta -> errore
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        ADDStat("nerrors", 1);
        ADDStat("nnotdelivered", -utentiConnessi->nConnessi);
        return -1;
    }

    icl_entry_t * bucket;
    icl_entry_t * currUsr;
    int posReceiver;
    int i;


    m.hdr.op = TXT_MESSAGE; // è il messaggio che devo inviare a tutti gli altri client

    for (i=0; i<utentiRegistrati->hash->nbuckets; i++) {
        LOCKPosRegistrati(i);
        bucket = utentiRegistrati->hash->buckets[i];
        for (currUsr=bucket; currUsr!=NULL; currUsr=currUsr->next) {
            if(strcmp(currUsr->key, m.hdr.sender) != 0){ // altimenti si invierebbe il messaggio da solo
                if((posReceiver = getUsrNickname(currUsr->key)) != -1){ // se è connesso gli invio il messaggio.
                    LOCKfd(posReceiver);
                    sendRequest(utentiConnessi->arr[posReceiver].fd, &m);
                    UNLOCKfd(posReceiver);
                }
                // In ogni caso aggiungo il messaggio alla sua history
                inserisci((coda_circolare_s *)currUsr->data, copyMessage(m));
            }
        }
        UNLOCKPosRegistrati(i);
    }
    UNLOCKConnessi();
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    return 0;
}

int postfile_op(long fd, message_t m){

    message_t r;
    int posSender, posReceiver;

    LOCKRegistrati(m.hdr.sender);
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    LOCKConnessi();
    if((posSender = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    UNLOCKRegistrati(m.hdr.sender);
    // TODO potrei pure non vedere se "è andata a buon fine, tanto il messaggio lo devo inviare comunque"
    LOCKfd(posSender);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[posSender].nickname) != 0 || utentiConnessi->arr[posSender].fd != fd){
        UNLOCKfd(posSender);
        UNLOCKConnessi();
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    /* Leggo il file */
    message_t file;

    char * path = malloc(sizeof(char) * (strlen(configurazione.DirName) + 1 + m.data.hdr.len + 1));
    strcat(path, configurazione.DirName);
    strcat(path, "/"); // TODO controllare errori
    strcat(path, m.data.buf); // TODO controllare errori
    printf("PATH: %s\n", path);
    //checkDirectory(configurazione.DirName);

    if(access(path, F_OK) != -1) { // se esiste già un file con quel nome -> torno errore OP_FAIL
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        return -1;
    }
    readData(fd, &(file.data)); // Leggo il file vero e proprio
    // file.data.hdr.len corrisponde alla dimensione del file in byte
    if((file.data.hdr.len / 1024) > configurazione.MaxFileSize){
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        return -1;
    }
    int fdNewFile;
    if((fdNewFile = open(path, O_CREAT | O_WRONLY, 0777)) == -1){ // TODO controllare errori
        perror("Errore creazione file\n");
        exit -1;
    }
    write(fdNewFile, file.data.buf, file.data.hdr.len);
    close(fdNewFile);
    // File creato

    /* Controllo se è registrato. Se non è registrato elimino il file e torno errore */
    coda_circolare_s * h;
    LOCKRegistrati(m.data.hdr.receiver);
    if((h = (coda_circolare_s *)icl_hash_find(utentiRegistrati->hash, m.data.hdr.receiver)) == NULL){
        UNLOCKRegistrati(m.data.hdr.receiver);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
        UNLOCKfd(posSender);
        remove(path); // TODO controllare errori
        return -1;
    }

    /* Lo comunico all'utente*/
    message_t toSend;
    setHeader(&(toSend.hdr), FILE_MESSAGE, m.hdr.sender);
    setData(&(toSend.data), "", m.data.buf, m.data.hdr.len);

    //LOCKConnessi(); // TODO attenzione al deadlock, forse non posso rilasciarla sopra
    if((posReceiver = getUsrNickname(m.data.hdr.receiver)) != -1){ // se è connesso gli mando subito un messaggio
        LOCKfd(posReceiver);
        sendRequest(utentiConnessi->arr[posReceiver].fd, &toSend);
        UNLOCKfd(posReceiver);
    }
    UNLOCKConnessi();

    // In ogni caso lo aggiungo sulla sua history

    inserisci(h, copyMessage(m));
    printf("Inserito nella history\n");
    UNLOCKRegistrati(m.data.hdr.receiver);
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(utentiConnessi->arr[posSender].fd, &(r.hdr));
    UNLOCKfd(posSender);
    return 0;
}

int getfile_op(long fd, message_t m){

    message_t r;
    int pos;

    LOCKRegistrati(m.hdr.sender);
    printf("Presa\n");
    if(!registrato(m.hdr.sender)){
        UNLOCKRegistrati(m.hdr.sender);
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    LOCKConnessi();
    if((pos = getUsrNickname(m.hdr.sender)) == -1){
        UNLOCKRegistrati(m.hdr.sender);
        UNLOCKConnessi();
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    UNLOCKRegistrati(m.hdr.sender);
    UNLOCKConnessi();
    LOCKfd(pos);
    if(strcmp(m.hdr.sender, utentiConnessi->arr[pos].nickname) != 0 || utentiConnessi->arr[pos].fd != fd){
        UNLOCKfd(pos);
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    char * path = malloc(sizeof(char) * (strlen(configurazione.DirName) + 1 + m.data.hdr.len + 1));
    strcat(path, configurazione.DirName);
    strcat(path, "/");
    strcat(path, m.data.buf);

    if(access(path, F_OK) == -1) {
        setHeader(&(r.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(utentiConnessi->arr[pos].fd, &(r.hdr));
        UNLOCKfd(pos);
        return -1;
    }

    int fdToSend;
    if((fdToSend = open(path, O_RDONLY)) == -1){
        perror("Errore apertura file"); // TODO gestione errore
        exit -1;
    }
    struct stat statBuf;
    if(fstat(fdToSend, &statBuf) == -1){
        perror("Errore stat file"); // TODO gestione errore
        exit -1;
    }

    if(!S_ISREG(statBuf.st_mode)){
        setHeader(&(r.hdr), OP_NO_SUCH_FILE, "");
        sendHeader(utentiConnessi->arr[pos].fd, &(r.hdr));
        UNLOCKfd(pos);
        return -1;
    }

    char * mappedfile = mmap(NULL, statBuf.st_size, PROT_READ,MAP_PRIVATE, fdToSend, 0);
    if(mappedfile == MAP_FAILED){
        perror("Errere mmap");
        exit -1; // TODO gestione errore
    }
    close(fdToSend);
    setHeader(&(r.hdr), OP_OK, "");
    setData(&(r.data), "", mappedfile, statBuf.st_size);
    sendRequest(utentiConnessi->arr[pos].fd, &r);
    UNLOCKfd(pos);
    munmap(mappedfile, statBuf.st_size);
    return 0;
}

void freeCoda(void * c){
    eliminaCoda((coda_circolare_s *)c);
}

/*
 * @ return -1 se non esiste un utente connesso con quel nickname
 *           altrimenti restituisce la posizione di quel nickname nell'array dei connessi
 */
int getUsrNickname(char * nickname){
    int i;
    for(i=0; i<configurazione.MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname) // controllo se il nickname è null
            if(strcmp(utentiConnessi->arr[i].nickname, nickname) == 0)
                return i;
    }
    return -1;
}

/*
 * @ return -1 se non esiste un utente connesso con quell'fd
 *           altrimenti restituisce la posizione di quell'fd nell'array dei connessi
 */
int getUsrFd(long fd){
    int i = 0;
    for(i=0; i<configurazione.MaxConnections; i++){
        if(utentiConnessi->arr[i].fd == fd)
            return i;
    }
    return -1;
}

/*
 * @return -1 se non ci sono posizioni libere
 *          altrimenti restituisce la prima posizione libera
 */
int getPosizioneLibera(){
    int i;
    for(i=0; i<configurazione.MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname == NULL && utentiConnessi->arr[i].fd == -1){ // basterebbe anche solo una delle due condizioni
            return i;
        }
    }
    return -1; // Non potrà mai succedere visto che verrà bloccato prima dal listener
}

message_t * copyMessage(message_t m){
    message_t * toAdd = malloc(sizeof(message_t)); // TODO gestire errore
    setHeader(&(toAdd->hdr), m.hdr.op, m.hdr.sender);
    setData(&(toAdd->data), m.data.hdr.receiver, strdup(m.data.buf), m.data.hdr.len);
    return toAdd;
}

int registrato(char * nickname){
    return icl_hash_find(utentiRegistrati->hash, (void *)nickname) != NULL;
}

char * makeListUsr(){
    int i = 0;
    char * r = calloc((MAX_NAME_LENGTH + 1) * utentiConnessi->nConnessi, sizeof(char)); // TODO controllare errore
    for(i=0; i<(MAX_NAME_LENGTH + 1) * utentiConnessi->nConnessi; i++){
        r[i] = '\0';
    }
    for(i=0; i<configurazione.MaxConnections; i++){
        if(utentiConnessi->arr[i].nickname != NULL && utentiConnessi->arr[i].fd != -1){
            concat(r, utentiConnessi->arr[i].nickname, ((MAX_NAME_LENGTH + 1) * i)); // basterebbe anche solo una delle due condizioni
        }
    }
    return r;
}

void concat(char * str1, char * str2, int first){ // Alla fine str1 = str1
    // str1 deve essere allocata in modo che possa contenere str2
    // first = primo spazio libero di str1 per copiare la stringa str2
    for(int i=0; i<strlen(str2); i++){
        str1[first + i] = str2[i];
    }
}
