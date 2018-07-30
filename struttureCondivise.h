#ifndef STRUTTURE_CONDIVISE_
#define STRUTTURE_CONDIVISE_

#include <pthread.h>
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneListe/linklist.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./lib/GestioneBoundedQueue/boundedqueue.h"
#include "./config.h"
#include "./parser.h"

#define THREAD_SAFE 1 // Serve per la libreria delle liste

typedef struct utente_connesso{
    char nickname[MAX_NAME_LENGTH];
    long fd;
    pthread_mutex_t fd_m;
} utente_connesso_s;


/* Variabili globali */
extern Queue_t * richieste;
extern config configurazione;
extern int nSocketUtenti;// = 0;
extern pthread_mutex_t nSocketUtenti_m;// = PTHREAD_MUTEX_INITIALIZER;
extern linked_list_t * utentiConnessi;
extern pthread_mutex_t hash_m[HASHSIZE / HASHGROUPSIZE];
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
extern icl_hash_t * utentiRegistrati;

static int find(void *item, size_t idx, void *user) {
    utente_connesso_s * u1 = (utente_connesso_s *)item;
    char * str = (char *)user;
    if(strcmp(u1->nickname, str) == 0) {
        return 0;
    } else return 1;
}

#endif
