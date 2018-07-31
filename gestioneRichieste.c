#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"

/* per tutte le funzioni: 0 successo, -1 errore */
// TODO boundequeue non è circolare e linklist.h non va bene, non fornisce i metodi necessari
int getIndexLockHash(char * key){
    int hash_val = (* ht->hash_function)(key) % ht->nbuckets;
    return hash_val % (HASHSIZE/HASHGROUPSIZE)
}

void freeElemBQueue(void * elem){
    message_t * m = (message_t *)elem;
    free(m->data.buf);
    free(m);
    /* Posso fare la free dell'intero messaggio dato che, in caso di POSTTEXTALL_OP,
       io farei tante copie del messaggio e le aggiungerei ad ogni client.
       Altrimenti se facessi un'unica copia del messaggio, dovrei vedere, prima di fare la free, se
       ci sono altre history di altri utenti che hanno il riferimento attivo*/
}
void freeBQueue(void * elem){
    BQueue_t * h = (BQueue_t *)elem;
    deleteBQueue(history, freeElemBQueue);
}

int register_op(long fd, message_t m){
    void * nickname = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1));
    strcpy((char *) nickname, m.hdr.sender);

    int indiceLock = getIndexLockHash(nickname);

    message_hdr_t r;
    pthread_mutex_lock(&hash_m[indiceLock]);
    if(icl_hash_find(utentiRegistrati, (void *)nickname) == NULL){ // Non esiste un utente con quel nickname, allora lo creo
        BQueue_t * history = initBQueue(configurazione.MaxHistMsgs);
        if(icl_hash_insert(utentiRegistrati, (void *)nickname, (void *)history) == NULL){ // Inserimento non andato a buon fine, allora rimuovo la coda che avevo creato per il nuovo utente
            pthread_mutex_unlock(&hash_m[indiceLock]);
            deleteBQueue(history, freeElemBQueue);
            free(nickname);
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            return -1;
        }else{
            pthread_mutex_unlock(&hash_m[indiceLock]);
            if(connect_op(fd, m) == -1){ // La connect non è andata a buon fine, allora devo annullare la registrazione
                pthread_mutex_lock(&hash_m[indiceLock]);
                icl_hash_delete(utentiRegistrati, nickname, free, freeBQueue); // Mi libera direttamente anche l'array circolare
                pthread_mutex_unlock(&hash_m[indiceLock]);
                free(nickname);
                return -1;
                // TODO devo mandare un messaggio o ci pensa la CONNECT_OP?
            }else return 0;
        }
    }else{ //Esiste già un utente con quel nickname, mando un messaggio di errore OP_NICK_ALREADY
        pthread_mutex_unlock(&hash_m[indiceLock]);
        setHeader(&r, OP_NICK_ALREADY, "");
        sendHeader(fd, &r);
        free(nickname);
        return -1;
    }
}

int connect_op(long fd, message_t m){
    void * nickname = (void *)(m.hdr.sender);

    int indiceLock = getIndexLockHash(nickname);
    message_hdr_t r;

    pthread_mutex_lock(&hash_m[indiceLock]);
    if(icl_hash_find(utentiRegistrati, (void *)nickname) == NULL){ // Non esiste un utente registrato con questo nickname, restituisco un messaggio di errore OP_NICK_UNKNOWN
        pthread_mutex_unlock(&hash_m[indiceLock]);
        setHeader(&r, OP_NICK_UNKNOWN, "");
        sendHeader(fd, &r);
        return -1;
    } else { // Esiste un utente registrato con questo nickname, allora lo aggiungo alla lista degli utenti connessi
        /* Non posso rilasciare la lock della hash prima di aver aggiunto nuovoUtente alla lista degli utenti connessi. Altrimenti se client1 facesse una richiesta UNREGISTER_OP per clientCorrente, il client1 non vedrebbe che clientCorrente è dentro la lista degli utenti connessi, quindi non lo eliminerebbe e resterebbe per sempre nella lista */
        if(list_foreach_value(utentiConnessi, find, nickname) < list_count(utentiConnessi)){ // Controllo se è già connesso
            //Trovato
            pthread_mutex_unlock(&hash_m[indiceLock]);
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            return -1;
        }

        utente_connesso_s * nuovoUtente = malloc(sizeof(utente_connesso_s));

        strncpy(nuovoUtente->nickname, nickname, MAX_NAME_LENGTH + 1);
        nuovoUtente->fd = fd;
        pthread_mutex_init(&(nuovoUtente->fd_m), NULL);
        if(list_push_value(utentiConnessi, (void *)nuovoUtente) == -1){ //Inserimento nella lista degli utenti connessi fallito
            pthread_mutex_unlock(&hash_m[indiceLock]);
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            pthread_mutex_destroy(&(nuovoUtente->fd_m));
            free(nuovoUtente);
            return -1;
        } else { // Inserimento nella lista degli utenti connessi andato a buon fine
            pthread_mutex_unlock(&hash_m[indiceLock]);
            if(usrlist_op(fd, m) == -1){ // usrlist_op fallita, allora elimino nuovoUtente dalla lista e lo libero
                // TODO eliminare l'utente dai connessi

                pthread_mutex_destroy(&(nuovoUtente->fd_m));
                free(nuovoUtente);
                return -1;
            }
        }
    }
}

int usrlist_op(long fd, message_t m){
    void * nickname = (void *)(m.hdr.sender);
    message_t r;
    // Controllo solo se l'utente è connesso, visto che per essere connessi bisogna per forza essere registrati
    if(list_foreach_value(utentiConnessi, find, nickname) < list_count(utentiConnessi)){ // Controllo se nickname è connesso
        // Nickname connesso, allora invio la lista degli utenti connessi

    }else{ // nickname non connesso, allora errore
        setHeader(&r, OP_FAIL, "");
        sendHeader(fd, &r);
        return -1;
    }

}

char * makeListUsr(){
    char * r = malloc(sizeof(char) * (MAX_NAME_LENGTH + 1) * list_count(utentiConnessi));
    list_lock(utentiConnessi);
}
