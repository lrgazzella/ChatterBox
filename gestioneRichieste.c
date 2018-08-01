#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"
#include "./lib/GestioneHistory/history.h"

void concat(char * str1, char * str2, int first);
char * makeListUsr();
/* per tutte le funzioni: 0 successo, -1 errore */

int getIndexLockHash(char * key){
    int hash_val = (* utentiRegistrati->hash->hash_function)(key) % utentiRegistrati->hash->nbuckets;
    return hash_val % (HASHSIZE/HASHGROUPSIZE);
}

int register_op(long fd, message_t m){
    void * nickname = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1));
    strcpy((char *) nickname, m.hdr.sender);

    int indiceLock = getIndexLockHash(nickname);

    message_hdr_t r;
    pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock]));
    if(icl_hash_find(utentiRegistrati->hash, (void *)nickname) == NULL){ // Non esiste un utente con quel nickname, allora lo creo
        history_s * hist = initHistory();

        if(icl_hash_insert(utentiRegistrati->hash, (void *)nickname, (void *)hist) == NULL){ // Inserimento non andato a buon fine, allora rimuovo la coda che avevo creato per il nuovo utente
            pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
            deleteHistory(hist);
            free(nickname);
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            return -1;
        }else{
            pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
            if(connect_op(fd, m) == -1){ // La connect non è andata a buon fine, allora devo annullare la registrazione
                pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock])); // TODO controllare se nickname è ancora dentro la hash
                if(icl_hash_find(utentiRegistrati->hash, (void *)nickname) != NULL){
                    icl_hash_delete(utentiRegistrati->hash, nickname, free, deleteHistory); // Mi libera direttamente anche la history
                    free(nickname); // Se non è più nella hash table, vuol dire che un altro thread del pool lo ha eliminato e quindi ha pure liberato il nickname
                }
                pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
                return -1;
                // TODO devo mandare un messaggio o ci pensa la CONNECT_OP?
            }else {
                printf("REGISTRATO\n");
                return 0;
            }
        }
    }else{ //Esiste già un utente con quel nickname, mando un messaggio di errore OP_NICK_ALREADY
        pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
        setHeader(&r, OP_NICK_ALREADY, "");
        sendHeader(fd, &r);
        free(nickname);
        return -1;
    }
}

int connect_op(long fd, message_t m){
    printf("INIZIATA CONNESSIONE\n");
    void * nickname = (void *)(m.hdr.sender);

    int indiceLock = getIndexLockHash(nickname);
    message_hdr_t r;

    pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock]));
    if(icl_hash_find(utentiRegistrati->hash, (void *)nickname) == NULL){ // Non esiste un utente registrato con questo nickname, restituisco un messaggio di errore OP_NICK_UNKNOWN
        pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
        setHeader(&r, OP_NICK_UNKNOWN, "");
        sendHeader(fd, &r);
        return -1;
    } else { // Esiste un utente registrato con questo nickname, allora lo aggiungo alla lista degli utenti connessi
        /* Non posso rilasciare la lock della hash prima di aver aggiunto nuovoUtente alla lista degli utenti connessi. Altrimenti se client1 facesse una richiesta UNREGISTER_OP per clientCorrente, il client1 non vedrebbe che clientCorrente è dentro la lista degli utenti connessi, quindi non lo eliminerebbe e resterebbe per sempre nella lista */

        pthread_mutex_lock(&(utentiConnessi->list_m));
        if(list_find(utentiConnessi->list, nickname) != NULL){ // Controllo se è già connesso
            // Esisite un utente già connesso con lo stesso nickname
            pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
            pthread_mutex_unlock(&(utentiConnessi->list_m));
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            return -1;
        }
        /* Creazione nuovo utente */
        list_node_t * user;
        utente_connesso_s * nuovoUtente = malloc(sizeof(utente_connesso_s));
        strncpy(nuovoUtente->nickname, nickname, MAX_NAME_LENGTH + 1);
        nuovoUtente->fd = fd;
        pthread_mutex_init(&(nuovoUtente->fd_m), NULL);

        if((user = list_rpush(utentiConnessi->list, list_node_new((void *)nuovoUtente))) == NULL){ //Inserimento nella lista degli utenti connessi fallito
            pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
            pthread_mutex_unlock(&(utentiConnessi->list_m));
            // L'unico caso in cui list_rpush torni NULL è quando gli si passa un nodo NULL, ovvero list_node_new non è riuscita a creare un nodo (errore nella malloc). Allora non devo fare nemmeno la free
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            pthread_mutex_destroy(&(nuovoUtente->fd_m));
            free(nuovoUtente);
            return -1;
        } else { // Inserimento nella lista degli utenti connessi andato a buon fine
            pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
            pthread_mutex_unlock(&(utentiConnessi->list_m));
            if(usrlist_op(fd, m) == -1){ // usrlist_op fallita, allora elimino nuovoUtente dalla lista e lo libero
                pthread_mutex_lock(&(utentiConnessi->list_m));
                list_node_t * cercato;
                if((cercato = list_find(utentiConnessi->list, nickname)) != NULL) { // Ho rilasciato e ripreso la lock, quindi nel mezzo potrebbe essere arrivata una richiesta di DISCONNECT_OP di quel nickname. Devo quindi vedere se anora appartiene alla lista. Se appartiene lo rimuovo e lo libero, altrimenti è già stato liberato nella funzione che gestisce la DISCONNECT_OP
                    list_remove(utentiConnessi->list, list_find(utentiConnessi->list, nickname)); // Non genera errori.
                    pthread_mutex_destroy(&(nuovoUtente->fd_m));
                    free(nuovoUtente);
                }
                pthread_mutex_unlock(&(utentiConnessi->list_m));
                return -1;
            }else{
                printf("CONNESSO\n");
                return 0;
            }
        }
    }
}

int usrlist_op(long fd, message_t m){
    printf("SPEDIZIONE LISTA UTENTI\n");
    void * nickname = (void *)(m.hdr.sender);
    message_t r;
    pthread_mutex_lock(&(utentiConnessi->list_m));
    if(list_find(utentiConnessi->list, nickname) != NULL){ // Controllo se l'utente è connesso (e quindi anche registrato). Se è connesso bene, altrimenti ritorno errore
        // Esisite un utente già connesso con lo stesso nickname
        char * listStr = makeListUsr();
        setHeader(&(r.hdr), 20, "");
        setData(&(r.data), "", listStr, (MAX_NAME_LENGTH + 1) * utentiConnessi->list->len);
        pthread_mutex_unlock(&(utentiConnessi->list_m));
        sendRequest(fd, &r);
        printf("SPEDITA\n");
        return 0;
    }else{
        pthread_mutex_unlock(&(utentiConnessi->list_m));
        setHeader(&(r.hdr), OP_FAIL, "");
        sendHeader(fd, &(r.hdr));
        return -1;
    }
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
