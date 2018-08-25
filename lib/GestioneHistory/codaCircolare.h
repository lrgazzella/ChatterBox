/** @file codaCircolare.h
  * @author Lorenzo Gazzella 546890
  * @brief File che contiene la dichiarazione delle funzioni per la gestione di una coda circolare
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
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

/**
  * @function initCodaCircolare
  * @brief Funzione che inizializza una coda circolare di una determinata lunghezza
  *
  * @param maxlung Lunghezza coda circolare
  * @param f Funzione che libera la memoria occupata da un elemento all'interno della coda circolare
  *
  * @return Coda circolare inizializzata
  */
coda_circolare_s * initCodaCircolare(int maxlung, void (*f)(void * elem));
/**
  * @function vuota
  * @brief Funzione booleana utilizzata per controllare se la coda è vuota
  *
  * @param c Coda circolare da controllare
  * @return 1 nel caso in cui la coda sia vuota, 0 altirmenti
  */
int vuota(coda_circolare_s * c);
/**
  * @function lung
  * @brief Funzione restiruisce il numero di elementi nella coda
  *
  * @param c Coda circolare da controllare
  * @return Numero di elementi nella coda
  */
int lung(coda_circolare_s * c);
/**
  * @function inserisci
  * @brief Funzione che inserisce un elemento in fondo a una coda circolare
  *
  * @param c Coda circolare da modificare
  * @param elem Elemento da inserire
  * @return 1 in caso di successo, 0 altrimenti
  */
int inserisci(coda_circolare_s * c, void * elem);
/**
  * @function elimina
  * @brief Funzione che elimina un elemento in testa ad una coda circolare
  *
  * @param c Coda circolare da modificare
  * @return 1 in caso di successo, 0 altrimenti
  */
int elimina(coda_circolare_s * c);
/**
  * @function eliminaCoda
  * @brief Funzione elimina una coda circolare
  *
  * @param c Coda circolare da eliminare
  * @return 1 in caso di successo, 0 altrimenti
  */
int eliminaCoda(coda_circolare_s * c);
/**
  * @function initIteratore
  * @brief Funzione inizializza un iteratore sulla coda circolare
  *
  * @param c Coda circolare da modificare
  * @return Iteratore sulla coda circolare inizializzato
  */
coda_circolare_iteratore_s * initIteratore(coda_circolare_s * c);
/**
  * @function next
  * @brief Funzione che restituisce un elemento della coda circolare
  *
  * @param i Iteratore
  * @return Elemento della coda circolare
  */
void * next(coda_circolare_iteratore_s * i);
/**
  * @function eliminaIteratore
  * @brief Funzione che elimina un iteratore
  *
  * @param i Iteratore
  * @return Iteratore da eliminare
  */
void eliminaIteratore(coda_circolare_iteratore_s * i);

#endif
