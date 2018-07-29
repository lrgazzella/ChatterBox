#include "./lib/GestioneBoundedQueue/boundedqueue.h"
#include "./parser.h"

int register_op(long fd, message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati,  linked_list_t * connessi, config configurazione){
    /*
        controllo se esiste un utente con quel nickname
        se non esiste creo un nuovo utente (struct e quindi devo allocare un array circolare)
        Invio la lista degli utenti
    */

    // Devo lockare la hashtable
    int hash_val = (* registrati->hash_function)(key) % registrati->nbuckets;
    int indiceLock = hash_val % (HASHSIZE/HASHGROUPSIZE);

    if(icl_hash_find(registrati, (void *)m.hdr.sender) == NULL){
        // Non esiste un utente con quel nickname, allora lo creo
        BQueue_t * history = initBQueue(configurazione.MaxHistMsgs);
        if(icl_hash_insert(registrati, (void *)m.hdr.sender, (void *)history) == NULL)
            pthread_mutex_unlock(lockRegistrati[indiceLock]);
            return -1;
        else{
            pthread_mutex_unlock(lockRegistrati[indiceLock]);
            connect_op(fd, m, registrati, lockRegistrati, connessi, configurazione); // la connect_op invia anche la lista degli utenti online
        }
    }

}
/* utente_connesso_s nuovoUtente;
nuovoUtente.nickname = m.hdr.sender;
nuovoUtente.fd = fd;
pthread_mutex_init(&(nuovoUtente.fd_m), NULL); */
