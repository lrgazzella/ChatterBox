/** @file utility.h
  * @author Lorenzo Gazzella 546890
  * @brief Contiene alcune funzioni di utilità.
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#ifndef UTILITY_H_
#define UTILITY_H_


/** @brief Macro che controlla se un certo valore s è uguale a -1. Se è uguale stampa un messaggio di errore e termina */
#define ec_meno1(s,m)  if ((s) == - 1) {\
    perror(m);\
    exit(EXIT_FAILURE);\
}
/** @brief Macro che controlla se un certo valore s è uguale a NULL. Se è uguale stampa un messaggio di errore e termina */
#define ec_null(s,m)  if ((s) == NULL) {\
    perror(m);\
    exit(EXIT_FAILURE);\
}
/** @brief Macro che controlla se un certo valore s è uguale a NULL. Se è uguale stampa un messaggio di errore e ritorna -1 */
#define ec_null_return(s,m) if ((s) == NULL) {\
    perror(m);\
    return -1;\
}
/** @brief Macro che controlla se un certo valore s è uguale a -1. Se è uguale stampa un messaggio di errore e ritorna -1 */
#define ec_meno1_return(s,m) if ((s) == -1) {\
    perror(m);\
    return -1;\
}
/** @brief Macro che controlla se un certo valore s è uguale a -1. Se è uguale stampa un messaggio di errore, esegue un comando e termina*/
#define ec_meno1_c(s,m,c) if ((s) == -1) {\
    perror(m);\
    c;\
}
/** @brief Macro che controlla se un certo valore s è uguale a NULL. Se è uguale stampa un messaggio di errore, esegue un comando e termina*/
#define ec_null_c(s,m,c) if ((s) == NULL) {\
    perror(m);\
    c;\
}

#include <errno.h>
#include <unistd.h>

/** @brief Macro che gestisce la lettura da un fd*/
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
    	if ((r=read((int)fd , bufptr,left)) == -1) {
    	    if (errno == EINTR) continue;
    	    return -1;
    	}
    	if (r == 0) return 0;   // gestione chiusura socket
      left    -= r;
    	bufptr  += r;
    }
    return size;
}
/** @brief Macro che gestisce la scrittura in un fd*/
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	     if ((r=write((int)fd ,bufptr,left)) == -1) {
         if (errno == EINTR) continue;
         return -1;
       }
       if (r == 0) return 0;
          left -= r;
	        bufptr  += r;
    }
    return 1;
}

#endif
