#ifndef STRUTTURE_CONDIVISE_
#define STRUTTURE_CONDIVISE_

#include <pthread.h>
#include <string.h>
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneListe/list.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./message.h"
#include "./config.h"
#include "./parser.h"
#include "./stats.h"


typedef struct utente_connesso{
    char * nickname; // Deve essere una stringa di lunghezza massima: [MAX_NAME_LENGTH + 1];
    long fd;
    pthread_mutex_t fd_m;
} utente_connesso_s;

typedef struct array_struct{
    utente_connesso_s * arr; // Contiene tutti gli utenti connessi
    int nConnessi;
    pthread_mutex_t arr_m;
} array_s;

typedef struct connessi{
    int contatore;
    pthread_mutex_t contatore_m;
} connessi_s;

typedef struct hash{
    icl_hash_t * hash;
    pthread_mutex_t hash_m[HASHSIZE / HASHGROUPSIZE];
} hash_s;

typedef struct stat_struct{
    struct statistics stat;
    pthread_mutex_t stat_m;
} stat_s;


/* Variabili globali */
extern Queue_t * richieste;
extern config configurazione;
extern hash_s * utentiRegistrati;
extern array_s * utentiConnessi;
extern connessi_s * nSock;
extern stat_s * statistiche;

static int getIndexLockHash(char * key){
    int hash_val = (* utentiRegistrati->hash->hash_function)(key) % utentiRegistrati->hash->nbuckets;
    return hash_val % (HASHSIZE/HASHGROUPSIZE);
}
// Dato un indice di buca della hash, prende la lock relativa
static void LOCKPosRegistrati(int pos){
    pthread_mutex_lock(&(utentiRegistrati->hash_m[pos % (HASHSIZE/HASHGROUPSIZE)]));
}

static void UNLOCKPosRegistrati(int pos){
    pthread_mutex_unlock(&(utentiRegistrati->hash_m[pos % (HASHSIZE/HASHGROUPSIZE)]));
}

static void LOCKRegistrati (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock]));
}

static void UNLOCKRegistrati (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
}

static void LOCKConnessi (){
    pthread_mutex_lock(&(utentiConnessi->arr_m));
}

static void UNLOCKConnessi (){
    pthread_mutex_unlock(&(utentiConnessi->arr_m));
}

static void LOCKnSock(){
    pthread_mutex_lock(&(nSock->contatore_m));
}

static void UNLOCKnSock(){
    pthread_mutex_unlock(&(nSock->contatore_m));
}

static void LOCKfd(int i){ // Prende la lock sull'fd dell'utente in posizione i
    pthread_mutex_lock(&(utentiConnessi->arr[i].fd_m));
}

static void UNLOCKfd(int i){
    pthread_mutex_unlock(&(utentiConnessi->arr[i].fd_m));
}

static void LOCKStat(){
    pthread_mutex_lock(&(statistiche->stat_m));
}

static void UNLOCKStat(){
    pthread_mutex_unlock(&(statistiche->stat_m));
}

static void ADDStat(char * stat, int s){ // somma s alla statistica stat
    LOCKStat();
    if(strcmp(stat, "nusers") == 0){
        statistiche->stat.nusers += s;
    }else if(strcmp(stat, "nonline") == 0){
        statistiche->stat.nonline += s;
    }else if(strcmp(stat, "ndelivered") == 0){
        statistiche->stat.ndelivered += s;
    }else if(strcmp(stat, "nnotdelivered") == 0){
        statistiche->stat.nnotdelivered += s;
    }else if(strcmp(stat, "nfiledelivered") == 0){
        statistiche->stat.nfiledelivered += s;
    }else if(strcmp(stat, "nfilenotdelivered") == 0){
        statistiche->stat.nfilenotdelivered += s;
    }else if(strcmp(stat, "nerrors") == 0){
        statistiche->stat.nerrors += s;
    }
    UNLOCKStat();
}

static void freeMessage_t(void * a){
    message_t * m = (message_t *)a;
    free(m->data.buf);
    free(m);
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
