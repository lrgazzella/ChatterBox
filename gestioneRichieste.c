#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"

/* per tutte le funzioni: 0 successo, -1 errore */

int register_op(long fd, message_t m){ // TODO devo limitare la size del nickname?
    /*
        controllo se esiste un utente con quel nickname
        se non esiste creo un nuovo utente (struct e quindi devo allocare un array circolare)
        Invio la lista degli utenti
    */
    // Devo lockare la hashtable
    int hash_val = (*(utentiRegistrati->hash_function))(m.hdr.sender) % utentiRegistrati->nbuckets;
    int indiceLock = hash_val % (HASHSIZE/HASHGROUPSIZE);
    message_hdr_t r;
    printf("Hash calcolato\n");
    pthread_mutex_lock(&hash_m[indiceLock]);
    if(icl_hash_find(utentiRegistrati, (void *)m.hdr.sender) == NULL){
        // Non esiste un utente con quel nickname, allora lo creo
        BQueue_t * history = initBQueue(configurazione.MaxHistMsgs);
        if(icl_hash_insert(utentiRegistrati, (void *)m.hdr.sender, (void *)history) == NULL){
            pthread_mutex_unlock(&hash_m[indiceLock]);
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            return -1;
        }else{
            pthread_mutex_unlock(&hash_m[indiceLock]);
            return connect_op(fd, m); // la connect_op invia anche la lista degli utenti online
        }
    }else{ //Esiste già un utente con quel nickname, mando un messaggio di errore OP_NICK_ALREADY
        pthread_mutex_unlock(&hash_m[indiceLock]);
        setHeader(&r, OP_NICK_ALREADY, "");
        sendHeader(fd, &r);
        return -1;
    }
}

int connect_op(long fd, message_t m){
    printf("Config: %d\n", configurazione.MaxConnections);
    void * nickname = (void *)&(m.hdr.sender);
    printf("NBUCKETS: %d\n", utentiRegistrati->nbuckets);
    //unsigned int (*hash)(void*) = (* utentiRegistrati->hash_function);

    printf("NICKNAME: %s\n", (char *)nickname);
    int hash_val = 10; //hash(nickname); //(utentiRegistrati->hash_function(nickname)) % utentiRegistrati->nbuckets;
    int indiceLock = hash_val % (HASHSIZE/HASHGROUPSIZE);

    message_hdr_t r;
    pthread_mutex_lock(&hash_m[indiceLock]);
    if(icl_hash_find(utentiRegistrati, (void *)m.hdr.sender) == NULL){
        // Non esiste, restituisco un messaggio di errore OP_NICK_UNKNOWN
        pthread_mutex_unlock(&hash_m[indiceLock]);
        setHeader(&r, OP_NICK_UNKNOWN, "");
        sendHeader(fd, &r);
        return -1;
    } else { // Esiste, allora lo aggiungo alla lista degli utenti connessi
        /* Non posso rilasciare la lock della hash prima di aver aggiunto nuovoUtente alla lista degli utenti connessi.
           Altrimenti se client1 facesse una richiesta UNREGISTER_OP per clientCorrente,
           il client1 non vedrebbe che clientCorrente è dentro la lista degli utenti connessi, quindi non lo eliminerebbe
           e resterebbe per sempre nella lista*/
        utente_connesso_s nuovoUtente;
        strncpy(nuovoUtente.nickname, m.hdr.sender, MAX_NAME_LENGTH); //TODO MAX_NAME_LENGTH o MAX_NAME_LENGTH+1?
        nuovoUtente.fd = fd;
        pthread_mutex_init(&(nuovoUtente.fd_m), NULL); //TODO controllare che l'utente non sia già connesso
        if(list_push_value(utentiConnessi, (void *)(&nuovoUtente)) == -1){
            setHeader(&r, OP_FAIL, "");
            sendHeader(fd, &r);
            pthread_mutex_unlock(&hash_m[indiceLock]);
            return -1;
        } else {
            pthread_mutex_unlock(&hash_m[indiceLock]);
            return usrlist_op(fd, m);
        }

    }
}

int usrlist_op(long fd, message_t m){
    message_t r;
    setHeader(&(r.hdr), OP_OK, "");
    setData(&(r.data), "", "GianniMarioLuigi", 16);
    sendRequest(fd, &r);
    return 0;
    /*toFind_s * f;
    f->nickname = m.hdr.sender;
    f->trovato = 0;
    list_foreach_value(utentiConnessi, find, (void *)f);
    if(f->trovato == 0){ // non esiste un utente connesso con quel nickname
        setHeader(&(r.hdr), OP_NICK_UNKNOWN, "");
        sendHeader(fd, &r);
        return -1;
    }else{
        int i;
        int numeroUtenti = list_count(utentiConnessi);
        char * listaUtenti = malloc(sizeof(char) * MAX_NAME_LENGTH * numeroUtenti);
        for(i=0; i<numeroUtenti; i++){
            utente_connesso_s * u = list_pick_value(utentiConnessi, i+1);
        }
        setHeader(&(r.hdr), OP_OK, "");
        setData(&(m.data), "", listaUtenti, strlen(listaUtenti));
        sendHeader(fd, &r);
    }*/
}
