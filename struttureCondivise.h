#ifndef STRUTTURE_CONDIVISE_
#define STRUTTURE_CONDIVISE_

#include <pthread.h>
#include <string.h>
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneListe/list.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./lib/GestioneBoundedQueue/boundedqueue.h"
#include "./config.h"
#include "./parser.h"

//int cmpNicknameElemList(void * a, void * b); // da settare prima di fare una ricerca per nickname

//int cmpFdElemList(void * a, void * b); // da settare prima di fare una ricerca per fd

//void freeUtente_Connesso_s(void * a);

typedef struct utente_connesso{
    char nickname[MAX_NAME_LENGTH + 1];
    long fd;
    pthread_mutex_t fd_m;
} utente_connesso_s;

typedef struct list_struct{
    list_t * list;
    pthread_mutex_t list_m;
} list_s;

typedef struct connessi{
    int contatore;
    pthread_mutex_t contatore_m;
} connessi_s;

typedef struct hash{
    icl_hash_t * hash;
    pthread_mutex_t hash_m[HASHSIZE / HASHGROUPSIZE];
} hash_s;


/* Variabili globali */
extern Queue_t * richieste;
extern config configurazione;
extern hash_s * utentiRegistrati;
extern list_s * utentiConnessi;
extern connessi_s * nSock;

static int getIndexLockHash(char * key){
    int hash_val = (* utentiRegistrati->hash->hash_function)(key) % utentiRegistrati->hash->nbuckets;
    return hash_val % (HASHSIZE/HASHGROUPSIZE);
}

static void LOCKHash (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock]));
}

static void UNLOCKHash (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
}

static void LOCKList (){
    pthread_mutex_lock(&(utentiConnessi->list_m));
}

static void UNLOCKList (){
    pthread_mutex_unlock(&(utentiConnessi->list_m));
}

static void LOCKnSock(){
    pthread_mutex_lock(&(nSock->contatore_m));
}

static void UNLOCKnSock(){
    pthread_mutex_unlock(&(nSock->contatore_m));
}

static int cmpNicknameElemList(void * a, void * b){
    // a è una stringa (nickname)
    // b è un utente_connesso_s
    return !strcmp((char *)a, ((utente_connesso_s *)b)->nickname);
}

static int cmpFdElemList(void * a, void * b){
    // a è un intero (fd)
    // b è un utente_connesso_s
    return (*((int *)a)) == ((utente_connesso_s *)b)->fd;
}

static void freeUtente_Connesso_s(void * a){ // da settare quando si crea una nuova lista di utente_connesso_s
    utente_connesso_s * aa = (utente_connesso_s *)a;
    pthread_mutex_destroy(&(aa->fd_m));
    free(aa);
}

/*
  utenti connessi a cosa serve?
  ogni volta che un utente mi fa una CONNECT_OP lo devo aggiungere alla lista.
  quando farà una richiesta in cui serve essere connessi, dovrò controllare se è presente
  quando un altro utente invia un messaggio a un client devo verificare se è connesso, semmai glielo posso mandare.
    In ogni caso devo aggiungere il messaggio nella history del ricevente
  quando un utente registrato  si disconnette dovrò toglierlo dalla lista
  QUINDI:
      - lettura e scrittura in mutua esclusione
      - serve una cond? NO
  OSS: utentiConnessi non sarà mai più grande delle socket attive (MaxConnections)
*/


#endif
