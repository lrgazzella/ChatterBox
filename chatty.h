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
#include "./lib/GestioneListe/list.h"
#include "./lib/GestioneHashTable/icl_hash.h"
#include "./stats.h"
#include "./utility.h"
#include "./connections.h"
#include "./config.h"
#include "./gestioneRichieste.h"



/* Definizione funzioni */
static void * listener(void * arg);
static void * pool(void * arg);
static void * segnali(void * arg);
int createSocket();
int aggiorna(fd_set * set, int max);
int isPipe(int fd);
void printRisOP(message_t m, int ok);
void stopAllThread(int segnali, int listener, int nThreadAttivi);
void joinAllThread();
void stopPool(int nThreadAttivi);
/* Funzioni init */
int initHashLock();
int initRegistrati();
int initDirFile();
int initNSock();
int initStat();
int initConnessi();
/* Funzioni cleanup */
void cleanupConfigurazione();
void cleanupRichieste();
void cleanupStat();
void cleanupNSock();
void cleanupRegistrati();
void cleanupConnessi();
void cleanupPipe();
void cleanupThreadId();

/* Funzione di comparazione usata nella tabella hash */
static inline int compareString( void *key1, void *key2  ) {
    return !strcmp((char *) key1, (char *) key2);
}

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

#endif
