/** @file chatty.h
  * @author Lorenzo Gazzella 546890
  * @brief File che contiene la dichiarazione di tutte le funzioni utilizzate nel file chatty.c
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#ifndef CHATTY_
#define CHATTY_
#define DEBUG 1

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
#include "./gestioneRichieste.h"
#include "./parser.h"
#include "./lib/GestioneQueue/queue.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./stats.h"
#include "./utility.h"
#include "./connections.h"
#include "./config.h"


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
/**
  * @function printRisOP
  * @brief Funzione che in base alla richiesta e al valore ritornato dalla funzione che l'ha gestista, stampa un determinato output
  *        La funzione stampa un messaggio solo se la macro DEBUG è stata definita.
  */
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



#endif
