#ifndef UTILITY_H_
#define UTILITY_H_


/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m)  if ((s) == - 1) {\
    perror(m);\
    exit(EXIT_FAILURE);\
}

#define ec_null(s,m)  if ((s) == NULL) {\
    perror(m);\
    exit(EXIT_FAILURE);\
}

/* controlla NULL; stampa errore e  termina la funzione   */
#define ec_null_return(s,m) if ((s) == NULL) {\
    perror(m);\
    return -1;\
}

#define ec_meno1_return(s,m) if ((s) == -1) {\
    perror(m);\
    return -1;\
}


#include <errno.h>
#include <unistd.h>


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
