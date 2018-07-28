#ifndef UTILITY_H_
#define UTILITY_H_
#endif /* UTILIY_H */

/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m)  if ((s) == - 1) {\
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
/* controlla -1; stampa errore ed esegue c */
#define ec_meno1_c(s,m,c) if ((s) == -1) {\
    perror(m);\
    c;\
}

#define ec_null_return_null(s,m) if ((s) == NULL) {\
    perror(m);\
    return NULL;\
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
