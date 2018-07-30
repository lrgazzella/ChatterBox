#ifndef STRUTTURE_CONDIVISE_
#define STRUTTURE_CONDIVISE_

#include <pthread.h>
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneListe/linklist.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./lib/GestioneBoundedQueue/boundedqueue.h"
#include "./config.h"
#include "./parser.h"

#define THREAD_SAFE 1 // Serve per la libreeria delle liste

typedef struct utente_connesso{
    char nickname[MAX_NAME_LENGTH];
    long fd;
    pthread_mutex_t fd_m;
} utente_connesso_s;

typedef struct toFind{
    char nickname[MAX_NAME_LENGTH];
    int trovato;
} toFind_s;

/* Variabili globali */
static Queue_t * richieste;
static config configurazione;
static int nSocketUtenti = 0;
static pthread_mutex_t nSocketUtenti_m = PTHREAD_MUTEX_INITIALIZER;
static linked_list_t * utentiConnessi;
static pthread_mutex_t hash_m[HASHSIZE / HASHGROUPSIZE];
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
static icl_hash_t * utentiRegistrati;

static int find(void *item, size_t idx, void *user) {
    utente_connesso_s * u = (utente_connesso_s *)item;
    toFind_s * f = (toFind_s *)user;
    if(strcmp(u->nickname, f->nickname) == 0) {
        f->trovato = 1;
        return 0;
    } else return 1;
}
#endif
