/** @file chatty.h
  * @author Lorenzo Gazzella 546890
  * @brief File che contiene la dichiarazione di tutte le funzioni utilizzate nel file chatty.c
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#ifndef CHATTY_
#define CHATTY_

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include "./struttureCondivise.h"
#include "./parser.h"
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./stats.h"
#include "./utility.h"
#include "./connections.h"
#include "./config.h"
#include "./gestioneRichieste.h"

/**
  * @function listener
  * @brief Funzione eseguita da un thread. Ha lo scopo di gestire l'interazione con i client.
  *        In particolare gestisce la connessione e disconnessione (della socket) di un client.
  *        Quando rivela che un client vuole fare una richiesta, questa verrà aggiunta alla coda delle richieste
  *
  * @param arrayPipe Array di pipe usate per la comunicazione con il pool.
  *                  Ogni posizione dell'array contiene la pipe per comunicare con un determinato pool.
  *                  In particolare conterrà ThreadsInPool pipe.
  */
static void * listener(void * arrayPipe);
/**
  * @function pool
  * @brief Funzione eseguita dai thread del pool. Ha lo scopo di gestire una richiesta del client.
  *        In particolare preleva dalla coda delle richiste una richiesta e la gestisce
  *
  * @param p Pipe con la quale può comunicare con il thread listener
  */
static void * pool(void * p);
/**
  * @function segnali
  * @brief Funzione eseguita dai un pool. Ha lo scopo di gestire i segnali del server.
  *        In particolare si mette in attesa di un segnale.
  *        Se il segnale arrivato è di tipo SIGUSR1 aggiorna il file delle statistiche
  *        Se il segnale arrivato è di tipo SIGQUIT, SIGTERM o SIGINT libera tutte le risorse in uso e fa terminare il server
  *
  * @param sgn_set Insieme dei segnali.
  */
static void * segnali(void * sgn_set);
/**
  * @function createSocket
  * @brief  Funzione che si occupa della creazione della socket.
  *
  * @return In caso di successo torna il file descriptor della nuova socket.
  *         In caso di errore torna -1
  */
int createSocket();
/**
  * @function aggiorna
  * @brief  Funzione che trova file descriptor più grande all'intero di un insieme di fd.
  *         Verrà chiamata a seguito di una cancellazione di un fd dall'insieme degli fd.
  * @param set Insieme di file descriptor
  * @param max Massimo attuale.
  *
  * @return Nuovo massimo
  */
int aggiorna(fd_set * set, int max);
/**
  * @function isPipe
  * @brief  Funzione che verifica se un fd è una pipe
  * @param fd fd da verificare
  *
  * @return: -1 se ci sono stati errori
  *           1 se fd è una pipe
  *           0 altrimenti
  */
int isPipe(int fd);
void printRisOP(message_t m, int ok);
/**
  * @function stopAllThread
  * @brief  Funzione che stoppa i thread indicati dai parametri

  * @param segnali Varrà 1 se la funzione deve stoppare il thread dei segnali, 0 altirmenti
  * @param listener Varrà 1 se la funzione deve stoppare il thread listener, 0 altirmenti
  * @param nThreadAttivi Numero dei thread del pool che deve stoppare
  */
void stopAllThread(int segnali, int listener, int nThreadAttivi);
/**
  * @function joinAllThread
  * @brief  Funzione effettua le join di tutti i thread del server
  */
void joinAllThread();
/**
  * @function stopPool
  * @brief  Funzione che stoppa un certo numero di thread del pool
  *
  * @param nThreadAttivi Numero di thread che deve stoppare
  */
void stopPool(int nThreadAttivi);

/**
  * @function initHashLock
  * @brief  Funzione che inizializza l'array (utentiRegistrati.hash_m) delle lock della tabella hash
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initHashLock();
/**
  * @function initRegistrati
  * @brief  Funzione che inizializza l'intera hash table degli utenti registrati e le relative lock
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initRegistrati();
/**
  * @function initDirFile
  * @brief  Funzione che inizializza la directory in cui si andranno a salvare tutti i file scambiati tra gli utenti
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initDirFile();
/**
  * @function initNSock
  * @brief  Funzione che inizializza il contatore di socket attive e la relativa lock.
  *         Se la cartella esiste già la svuota, altrimenti la crea
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initNSock();
/**
  * @function initStat
  * @brief  Funzione che inizializza la struct delle statistiche
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initStat();
/**
  * @function initConnessi
  * @brief  Funzione che inizializza l'array degli utenti connessi e le relative lock
  *
  * @return 0 in caso di successo, -1 altirmenti
  */
int initConnessi();

/**
  * @function cleanupConfigurazione
  * @brief  Funzione che libera la memoria occupata dalla struct delle configurazioni
  */
void cleanupConfigurazione();
/**
  * @function cleanupRichieste
  * @brief  Funzione che libera la memoria occupata dalla coda delle richieste
  */
void cleanupRichieste();
/**
  * @function cleanupStat
  * @brief  Funzione che libera la memoria occupata dalla struct delle statistiche
  */
void cleanupStat();
/**
  * @function cleanupNSock
  * @brief  Funzione che libera la memoria occupata dalla struct relativa al contatore di socket attive
  */
void cleanupNSock();
/**
  * @function cleanupRegistrati
  * @brief  Funzione che libera la memoria occupata hash degli utenti registrati
  */
void cleanupRegistrati();
/**
  * @function cleanupConnessi
  * @brief  Funzione che libera la memoria occupata dall'array degli utenti connessi
  */
void cleanupConnessi();
/**
  * @function cleanupPipe
  * @brief  Funzione che libera la memoria occupata dall'array di pipe usate per la comunicazione tra listener e pool
  */
void cleanupPipe();
/**
  * @function cleanupThreadId
  * @brief  Funzione che libera la memoria occupata dall'array degli id dei thread
  */
void cleanupThreadId();

/**
  * @function compareString
  * @brief  Funzione di comparazione usata nella tabella hash
  */
static inline int compareString( void *key1, void *key2  ) {
    return !strcmp((char *) key1, (char *) key2);
}

/**
  * @function usage
  * @brief  Funzione che stampa il corretto avviamento del server
  */
static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

#endif
