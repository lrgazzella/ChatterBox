#include <stdio.h>
#include "./codaCircolare.h"


coda_circolare_s * initCodaCircolare(int maxlung, void (*f)(void * elem)){
    coda_circolare_s * c = malloc(sizeof(coda_circolare_s));
    c->coda = malloc(sizeof(void *) * maxlung);
    c->lung = 0;
    c->testa = 0;
    c->maxlung = maxlung;
    c->free = f;
    return c;
}

int vuota(coda_circolare_s * c){
    return c->lung == 0;
}

int lung(coda_circolare_s * c){
    return c->lung;
}

int inserisci(coda_circolare_s * c, void * elem){ // inserisce un elemento in fondo alla coda
    if(!c) return 0;
    if(c->lung == c->maxlung){
        elimina(c);
    }
    c->coda[(c->testa + c->lung) % c->maxlung] = elem;
    (c->lung) ++;
    return 1;
}

int elimina(coda_circolare_s * c){ // elimina un elemento in testa alla coda
    if(!c) return 0;

    if(!vuota(c)){
        (* c->free)((void *)c->coda[c->testa]);
        c->testa = (c->testa + 1) % c->maxlung;
        c->lung = (c->lung) - 1;
    }
    return 1;
}

int eliminaCoda(coda_circolare_s * c){ // elimina tutta la coda
    if(!c) return 0;
    int i;
    int l = c->lung;
    for(i=0; i<l; i++){
        elimina(c);
    }
    free(c->coda);
    free(c);
    return 1;
}

coda_circolare_iteratore_s * initIteratore(coda_circolare_s * c){ // se la coda è vuota torna null
    if(vuota(c)) return NULL;
    coda_circolare_iteratore_s * i = malloc(sizeof(coda_circolare_iteratore_s));
    i->next = c->testa;
    i->codaCircolare = c;
    i->daLeggere = c->lung;
    return i;
}

void * next(coda_circolare_iteratore_s * i){
    if(i->daLeggere == 0) return NULL;

    void * r = i->codaCircolare->coda[i->next];
    i->next = (i->next + 1) % (i->codaCircolare->maxlung);
    i->daLeggere --;
    return r;
}

void eliminaIteratore(coda_circolare_iteratore_s * i){
    free(i);
}
