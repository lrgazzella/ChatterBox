/** @file struttureCondivise.h
  * @author Lorenzo Gazzella 546890
  * @brief File che contiene tutte le trutture dati utilizzate dal server
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#ifndef STRUTTURE_CONDIVISE_
#define STRUTTURE_CONDIVISE_

#include <pthread.h>
#include <string.h>
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./lib/GestioneHistory/codaCircolare.h"
#include "./message.h"
#include "./config.h"
#include "./parser.h"
#include "./stats.h"

/**
 *  @struct utente_connesso
 *  @brief Struttura utilizzata per memorizzare tutte le informazioni di un utente connesso
 *  @var nickname Nickname dell'utente. Lunghezza massima: [MAX_NAME_LENGTH + 1]
 *  @var fd fd con cui si può comunicare con l'utente
 *  @var fd_m lock relativa all'fd
 */
typedef struct utente_connesso{
    char * nickname;
    long fd;
    pthread_mutex_t fd_m;
} utente_connesso_s;

/**
 *  @struct array_struct
 *  @brief Struttura utilizzata per memorizzare l'insieme degli utenti connessi
 *  @var arr Insieme utenti connessi
 *  @var nConnessi Numero di utenti connessi
 *  @var arr_m lock relativa all'insieme degli utenti connessi
 */
typedef struct array_struct{
    utente_connesso_s * arr;
    int nConnessi;
    pthread_mutex_t arr_m;
} array_s;
/**
 *  @struct connessi
 *  @brief Struttura utilizzata per memorizzare un contatore in mutua esclusione. Verrà utilizzato per tenere traccia del numero di socket attive
 *  @var contatore Contatore
 *  @var contatore_m lock relativa al contatore
 */
typedef struct connessi{
    int contatore;
    pthread_mutex_t contatore_m;
} connessi_s;
/**
 *  @struct hash
 *  @brief Struttura utilizzata per memorizzare tutti gli utenti registrati
 *  @var hash Tabella hash per memorizzare gli utenti
 *  @var hash_m Array di lock relativo alla tabella hash.
 */
typedef struct hash{
    icl_hash_t * hash;
    pthread_mutex_t hash_m[HASHSIZE / HASHGROUPSIZE];
} hash_s;
/**
 *  @struct stat_struct
 *  @brief Struttura utilizzata per avere la struct delle statistiche in mutua esclusione
 *  @var stat Struct statistiche
 *  @var stat_m Lock relativa alla struct delle statistiche
 */
typedef struct stat_struct{
    struct statistics stat;
    pthread_mutex_t stat_m;
} stat_s;


/* Variabili globali */
extern Queue_t * richieste;
extern config * configurazione;
extern hash_s * utentiRegistrati;
extern array_s * utentiConnessi;
extern connessi_s * nSock;
extern stat_s * statistiche;

/**
 * @function getIndexLockHash
 * @brief Funzione che restituisce la posizione nell'array della lock (hash_m) relativa a una determinata chiave
 *
 * @param key Chiave
 *
 * @return Posizione nell'array della lock (hash_m) relativa alla chiave key
 */
static int getIndexLockHash(char * key){
    int hash_val = (* utentiRegistrati->hash->hash_function)(key) % utentiRegistrati->hash->nbuckets;
    return hash_val % (HASHSIZE/HASHGROUPSIZE);
}
// Dato un indice di buca della hash, prende la lock relativa

/**
 * @function LOCKPosRegistrati
 * @brief Funzione che dato un indice di buca (bucket) della hash, prende la lock relativa
 *
 * @param pos Indice di buca
 */
static inline void LOCKPosRegistrati(int pos){
    pthread_mutex_lock(&(utentiRegistrati->hash_m[pos % (HASHSIZE/HASHGROUPSIZE)]));
}

/**
 * @function UNLOCKPosRegistrati
 * @brief Funzione che dato un indice di buca (bucket) della hash, rilascia la lock relativa
 *
 * @param pos Indice di buca
 */
static inline void UNLOCKPosRegistrati(int pos){
    pthread_mutex_unlock(&(utentiRegistrati->hash_m[pos % (HASHSIZE/HASHGROUPSIZE)]));
}
/**
 * @function LOCKRegistrati
 * @brief Funzione che dato un nickname, prende la lock sull'array delle lock (hash_m) relativa a quel nickname
 *
 * @param nick Nickname
 */
static inline void LOCKRegistrati (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_lock(&(utentiRegistrati->hash_m[indiceLock]));
}
/**
 * @function UNLOCKRegistrati
 * @brief Funzione che dato un nickname, rilascia la lock sull'array delle lock (hash_m) relativa a quel nickname
 *
 * @param nick Nickname
 */
static inline void UNLOCKRegistrati (char * nick){
    int indiceLock = getIndexLockHash(nick);
    pthread_mutex_unlock(&(utentiRegistrati->hash_m[indiceLock]));
}
/**
 * @function LOCKConnessi
 * @brief Funzione che prende la lock sull'array degli utenti connessi
 *
 */
static inline void LOCKConnessi (){
    pthread_mutex_lock(&(utentiConnessi->arr_m));
}
/**
 * @function UNLOCKConnessi
 * @brief Funzione che rilascia la lock sull'array degli utenti connessi
 *
 */
static inline void UNLOCKConnessi (){
    pthread_mutex_unlock(&(utentiConnessi->arr_m));
}
/**
 * @function LOCKnSock
 * @brief Funzione che prende la lock sul contatore delle socket attive
 *
 */
static inline void LOCKnSock(){
    pthread_mutex_lock(&(nSock->contatore_m));
}
/**
 * @function UNLOCKnSock
 * @brief Funzione che rilascia la lock sul contatore delle socket attive
 *
 */
static inline void UNLOCKnSock(){
    pthread_mutex_unlock(&(nSock->contatore_m));
}
/**
 * @function LOCKfd
 * @brief Funzione che prende la lock sull fd in posizione i
 *
 */
static inline void LOCKfd(int i){ // Prende la lock sull'fd dell'utente in posizione i
    pthread_mutex_lock(&(utentiConnessi->arr[i].fd_m));
}
/**
 * @function UNLOCKfd
 * @brief Funzione che rilascia la lock sull fd in posizione i
 *
 */
static inline void UNLOCKfd(int i){
    pthread_mutex_unlock(&(utentiConnessi->arr[i].fd_m));
}
/**
 * @function LOCKStat
 * @brief Funzione che prende la lock sulla struct delle statistiche
 *
 */
static void LOCKStat(){
    pthread_mutex_lock(&(statistiche->stat_m));
}
/**
 * @function UNLOCKStat
 * @brief Funzione che rilascia la lock sulla struct delle statistiche
 *
 */
static void UNLOCKStat(){
    pthread_mutex_unlock(&(statistiche->stat_m));
}
/**
 * @function ADDStat
 * @brief Funzione che somma ad un certo campo della struct delle statistiche un certo valore
 *
 * @param stat Campo della struct da modificare
 * @param s Valore da sommare (anche negativo)
 */
static inline void ADDStat(char * stat, int s){ // somma s alla statistica stat
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
/**
 * @function freeCoda
 * @brief Funzione che libera una determinata coda circolare
 *
 * @param c Coda circolare che si vuole liberare
 */
static void freeCoda(void * c){
    eliminaCoda((coda_circolare_s *)c);
}
/**
 * @function freeMessage_t
 * @brief Funzione che libera determinato messaggio
 *
 * @param a Messaggio che si vuole liberare
 */
static inline void freeMessage_t(void * a){
    message_t * m = (message_t *)a;
    free(m->data.buf);
    free(m);
}

// TODO da riportarlo nella relazione finale
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
