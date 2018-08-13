#ifndef coda_circolare_
#define coda_circolare_

#include <stdlib.h>

typedef struct coda_circolare{
    void ** coda;
    int testa;
    int lung;
    int maxlung;
    void (*free)(void * elem);
} coda_circolare_s;

typedef struct coda_circolare_iteratore {
    coda_circolare_s * codaCircolare;
    int next;
    int daLeggere;
} coda_circolare_iteratore_s;


coda_circolare_s * initCodaCircolare(int maxlung, void (*f)(void * elem));

int vuota(coda_circolare_s * c);

int lung(coda_circolare_s * c);

int inserisci(coda_circolare_s * c, void * elem);

int elimina(coda_circolare_s * c);

int eliminaCoda(coda_circolare_s * c);

coda_circolare_iteratore_s * initIteratore(coda_circolare_s * c);

void * next(coda_circolare_iteratore_s * i);

void eliminaIteratore(coda_circolare_iteratore_s * i);

#endif
