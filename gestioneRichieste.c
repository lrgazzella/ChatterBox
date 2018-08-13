#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"
#include "./lib/GestioneHistory/codaCircolare.h"

void concat(char * str1, char * str2, int first);
char * makeListUsr();
int registrato(char * nickname);
list_node_t * getUtenteFd(long fd);
list_node_t * getUtenteNickname(char * nickname);
void setOrdinamentoLista(int (*match)(void *a, void *b));
message_t * copyMessage(message_t m);
void freeCoda(void * c);
char * intToString(int n);
/* per tutte le funzioni: 0 successo, -1 errore */

int register_op(long fd, message_t m){ // Prima di chiamarla bisogna fare la lock della hash
    void * nickname = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1));
    strcpy((char *) nickname, m.hdr.sender);

    message_t r;
    if(!registrato((char *)nickname)){ // Non esiste un utente con quel nickname, allora lo creo
        coda_circolare_s * hist = initCodaCircolare(configurazione.MaxMsgSize, freeMessage_t);

        if(icl_hash_insert(utentiRegistrati->hash, (void *)nickname, (void *)hist) == NULL){ // Inserimento non andato a buon fine, allora rimuovo la coda che avevo creato per il nuovo utente
            eliminaCoda(hist);
            free(nickname);
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            return -1;
        }else{
            LOCKList();
            if(connect_op(fd, m) == -1){ // La connect non è andata a buon fine, allora devo annullare la registrazione
                icl_hash_delete(utentiRegistrati->hash, nickname, free, freeCoda); // Mi libera direttamente anche la history
                UNLOCKList();
                free(nickname);
                return -1;
            }else {
                UNLOCKList();
                return 0;
            }
        }
    }else{ //Esiste già un utente con quel nickname, mando un messaggio di errore OP_NICK_ALREADY
        setHeader(&(r.hdr), OP_NICK_ALREADY, "");
        sendHeader(fd, &(r.hdr));
        free(nickname);
        return -1;
    }
}

int connect_op(long fd, message_t m){ // Prima di chiamarla bisogna fare la lock della hash e della lista degli utenti connessi
    void * nickname = (void *)(m.hdr.sender);

    message_t r;

    if(!registrato((char *)nickname)){ // Non esiste un utente registrato con questo nickname, restituisco un messaggio di errore OP_NICK_UNKNOWN
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    } else { // Esiste un utente registrato con questo nickname, allora lo aggiungo alla lista degli utenti connessi
        /* Non posso rilasciare la lock della hash prima di aver aggiunto nuovoUtente alla lista degli utenti connessi. Altrimenti se client1 facesse una richiesta UNREGISTER_OP per clientCorrente, il client1 non vedrebbe che clientCorrente è dentro la lista degli utenti connessi, quindi non lo eliminerebbe e resterebbe per sempre nella lista */

        if(getUtenteNickname(nickname) != NULL){ // Controllo se è già connesso
            // Esisite un utente già connesso con lo stesso nickname
            setHeader(&(r.hdr), OP_FAIL, ""); // Se non facessi questo controllo potrei dire, a un utente che ha sbagliato l'inserimento del nickame, che la connessione è andata a buon fine, mentre il nickname inserito è già connesso
            sendHeader(fd, &(r.hdr));
            return -1;
        }
        /* Creazione nuovo utente */
        list_node_t * userNode;
        utente_connesso_s * usr = malloc(sizeof(utente_connesso_s));
        strncpy(usr->nickname, nickname, MAX_NAME_LENGTH + 1);
        usr->fd = fd;
        pthread_mutex_init(&(usr->fd_m), NULL);

        if((userNode = list_rpush(utentiConnessi->list, list_node_new((void *)usr))) == NULL){ //Inserimento nella lista degli utenti connessi fallito
            // L'unico caso in cui list_rpush torni NULL è quando gli si passa un nodo NULL, ovvero list_node_new non è riuscita a creare un nodo (errore nella malloc). Allora non devo fare nemmeno la free
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr));
            freeUtente_Connesso_s(usr);
            return -1;
        } else { // Inserimento nella lista degli utenti connessi andato a buon fine
            if(usrlist_op(fd, m) == -1){ // usrlist_op fallita, allora elimino nuovoUtente dalla lista e lo libero
                list_remove(utentiConnessi->list, userNode); // Non genera errori. Dellaoca in automatico il nodo eliminato
                return -1;
            }else{
                return 0;
            }
        }
    }
}

int usrlist_op(long fd, message_t m){ // va chiamata con la lock sulla lista degli utenti connessi
    void * nickname = (void *)(m.hdr.sender);
    message_t r;
    if(!registrato((char *)nickname)){ // Controllo se l'utente è registrato. Se non è registrato gli mando un errore OP_NICK_UNKNOWN
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
    if(getUtenteNickname(nickname) != NULL){ // Controllo se l'utente è connesso. Se è connesso bene, altrimenti ritorno errore
        // Esisite un utente già connesso con lo stesso nickname
        char * listStr = makeListUsr();
        setHeader(&(r.hdr), 20, "");
        setData(&(r.data), "", listStr, (MAX_NAME_LENGTH + 1) * utentiConnessi->list->len);
        sendRequest(fd, &r); // TODO lockare fd
        return 0;
    }else{
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
}

int unregister_op(long fd, message_t m){ // va chiamata con la lock sulla tabella hash (blocco del receiver e del sender) e sulla lista degli utenti connessi
    void * nickname = (void *)(m.hdr.sender);
    void * daEliminare = (void *)(m.data.hdr.receiver);
    message_t r;

    if(!registrato((char *)nickname)){ // Se l'utente che vuole deregistrare non è registrato gli mando un messaggio di errore
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
    if(getUtenteNickname((char *)nickname) == NULL){ // Utente non connesso e quindi non registrato -> errore OP_FAIL. Oss: devo controllare se è registrato perchè se il client scrive un comando tipo "./client -l nomesocket -C Lorenzo -c Lorenzo -k Lorenzo", il parser non lo blocca perchè ci sono tutti gli elmenti ma poichè li manda in modo sequenziale, arriva prima la richiesta di deregistrazione di un client non ancora registrato
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }else{ // utente connesso e quindi registrato -> posso cancellare "daEliminare"

        if(!registrato((char *)daEliminare)){ // guardo se daEliminare è registrato. se non è registrato mando errore
            setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
            sendHeader(fd, &(r.hdr)); // TODO lockare fd
            return -1;
        }
        list_node_t * usr;
        if((usr = getUtenteNickname((char *)daEliminare)) != NULL){ // Controllo se daEliminare è connesso. semmai lo elimino dalla lista degli utenti connessi
            list_remove(utentiConnessi->list, usr);
        }

        if(icl_hash_delete(utentiRegistrati->hash, daEliminare, free, freeCoda) == -1){ // errore
            setHeader(&(r.hdr), OP_FAIL, "");
            sendHeader(fd, &(r.hdr)); // TODO lockare fd
            return -1;
        }else{
            setHeader(&(r.hdr), OP_OK, "");
            sendHeader(fd, &(r.hdr)); // TODO lockare fd
            return 0;
        }
    }
}

int disconnect_op(long fd){ // va chiamata con la lock sugli utenti connessi
    list_node_t * usr;
    if((usr = getUtenteFd(fd)) != NULL){
        list_remove(utentiConnessi->list, usr);
        return 0;
    }
    return -1;
}

int posttxt_op(long fd, message_t m){ // va chiamata con la lock sulla tabella hash e sulla lista degli utenti connessi
    void * sender = (void *)(m.hdr.sender);
    void * receiver = (void *)(m.data.hdr.receiver);

    message_t r;

    if(getUtenteNickname(sender) == NULL){ // Guardo se l'utente che sta inviando il messaggio è connesso e quindi registrato // TODO verificare se basta guarda se l'utente è connesso oppure serve anche guardare se è registrato. Questo solo per i messaggi di ritorno. Se non regiustrato OP_NICK_UNKNOWN. Se non connesso OP_FAIL
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
    coda_circolare_s * h;
    if((h = icl_hash_find(utentiRegistrati->hash, (void *)receiver)) == NULL){  // guardo se il ricevente è registrato. Se non è registrato errore
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    if(m.data.hdr.len > configurazione.MaxMsgSize){ // Controllo subito se la lunghezza del messaggio rispetta il file di configurazione. se non rispetta -> errore
        setHeader(&(r.hdr), OP_MSG_TOOLONG, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
    /* Invio messaggio */
    list_node_t * usrReceiver;
    m.hdr.op = TXT_MESSAGE; // Cambio l'op del messaggio
    if((usrReceiver = getUtenteNickname(receiver)) != NULL){ // Se il receiver è connesso glielo invio subito
        pthread_mutex_lock(&(((utente_connesso_s *)usrReceiver->val)->fd_m));
        sendRequest(((utente_connesso_s *)usrReceiver->val)->fd, &m); // TODO controllare errori
        pthread_mutex_unlock(&(((utente_connesso_s *)usrReceiver->val)->fd_m));
    }
    // In ogni caso aggiungo il messaggio alla history del receiver

    inserisci(h, copyMessage(m));
    setHeader(&(r.hdr), OP_OK, "");
    sendHeader(fd, &(r.hdr));
    return 0;
}

int getprevmsgs_op(long fd, message_t m){ // va chiamata con la lock sulla tabella hash e sulla lista degli utenti connessi
    void * sender = (void *)(m.hdr.sender);

    message_t r;
    coda_circolare_s * h;
    if((h = (coda_circolare_s *)icl_hash_find(utentiRegistrati->hash, sender)) == NULL){
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    if(getUtenteNickname((char *)sender) == NULL){ // Devo guardare se è connesso perchè potrebbe esserci il caso che il client mi mandi: "./client -l nomesocket -p -k nomeutente" e visto che la connessione potrebbe fallire, rischierei di consentire di far fare un'azione a un utente non connesso
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }

    /* Comunico al client quanti messaggi sono contenuti nella history */
    char * l = intToString(lung(h));
    printf("L: %s\n", l);
    setHeader(&(r.hdr), OP_OK, ""); // TODO invio lunghezza errato
    setData(&(r.data), "", l, strlen(l) + 1);
    // TODO lockare fd
    sendRequest(fd, &r);

    /* Invio di ogni messaggio */
    coda_circolare_iteratore_s * it = initIteratore(h);
    message_t * toSend;
    while((toSend = (message_t *)next(it)) != NULL){
        sendRequest(fd, toSend); // TODO controllare errori
    }
    eliminaIteratore(it);
    return 0;
}

void freeCoda(void * c){
    eliminaCoda((coda_circolare_s *)c);
}

char * intToString(int n){ // Prende un intero e restituisce una stringa. Lo spazio allocato per la stringa è uguale allo spazio necessario per allocare un carattere per il numero di cifre del numero
    int num = n;
    /* Calcolo il numero di cifre da cui è composo n */
    int numeroCifre = 0;
    while(n != 0) {
        n /= 10;
        numeroCifre++;
    }
    char * str = malloc(sizeof(char) * numeroCifre);
    sprintf(str, "%d", num);
    return str;
}

message_t * copyMessage(message_t m){
    message_t * toAdd = malloc(sizeof(message_t)); // TODO gestire errore
    setHeader(&(toAdd->hdr), m.hdr.op, m.hdr.sender);
    setData(&(toAdd->data), m.data.hdr.receiver, strdup(m.data.buf), m.data.hdr.len);
    return toAdd;
}

list_node_t * getUtenteFd(long fd){ // Da chiamara con la lock sulla lista degli utenti connessi
    setOrdinamentoLista(cmpFdElemList);
    return list_find(utentiConnessi->list, (void *)&fd);
}

void setOrdinamentoLista(int (*match)(void *a, void *b)){
    utentiConnessi->list->match = match;
}

list_node_t * getUtenteNickname(char * nickname){ // Da chiamara con la lock sulla lista degli utenti connessi
    setOrdinamentoLista(cmpNicknameElemList);
    return list_find(utentiConnessi->list, nickname);
}

int registrato(char * nickname){
    return icl_hash_find(utentiRegistrati->hash, (void *)nickname) != NULL;
}

char * makeListUsr(){
    list_iterator_t *it = list_iterator_new(utentiConnessi->list, LIST_HEAD);
    list_node_t *node;
    int i = 0;
    char * r = calloc((MAX_NAME_LENGTH + 1) * utentiConnessi->list->len, sizeof(char)); // TODO controllare errore
    while (node = list_iterator_next(it)) {
        utente_connesso_s * usr = ((utente_connesso_s *)node->val);
        concat(r, usr->nickname, ((MAX_NAME_LENGTH + 1) * i));
        i++;
    }
    list_iterator_destroy(it);
    return r;
}

void concat(char * str1, char * str2, int first){ // Alla fine str1 = str1 , str2
    // str1 deve essere allocata in modo che possa contenere str2
    // first = primo spazio libero di str1 per copiare la stringa str2
    for(int i=0; i<(MAX_NAME_LENGTH + 1); i++){
        str1[first + i] = str2[i];
    }
}
