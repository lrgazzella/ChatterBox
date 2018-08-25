/** @file parser.h
  * @author Lorenzo Gazzella 546890
  * @brief File che contiene tutte le funzioni necessarie al parsing di un file di configurazione
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#ifndef PARSER_H_
#define PARSER_H_

typedef struct config_s{
    char * UnixPath;
    int MaxConnections;
    int ThreadsInPool;
    int MaxMsgSize;
    int MaxFileSize;
    int MaxHistMsgs;
    char * DirName;
    char * StatFileName;
} config;

/**
  * @function parse
  *
  * @brief Prende un file e inserisce in configs tutte le configurazioni presenti nel file.
  *        Se ci sono due parametri uguali nello stesso file di configurazione, verrà preso il valore dell'ultimo
  * @param nomeFile Path del file che si vuole parsare
  * @param configs Struttura dati su cui si vogliono memorizzare i valori parsati
  *
  * @return 0 se non ci sono stati errori
  *         -1 se ci sono stati errori
  */
int parse(char * nomeFile, config * configs);
/**
  * @function makeConfig
  *
  * @brief Funzione setta a un certo campo della struct un determinato valore
  * @param parametro Campo della struct che si vuole modificare
  * @param value Valore che si vuole settare al campo
  * @param configs Struct che si vuole modificare
  * @return 0 se non ci sono stati errori
  *         -1 se ci sono stati errori
  */
int makeConfig(char * parametro, char * value, config * configs);
/**
  * @function check
  *
  * @brief Funzione che elimina gli tutti gli spazi, gli '\t' e gli '\n' da una determinata stringa
  * @param configs Struct che si vuole controllare
  * @return 0 se non ci sono stati errori
  *         -1 se ci sono stati errori
  */
void RemoveSpaces(char* source);
/**
  * @function initParseCheck
  *
  * @brief Funzione che chiama in ordine: init, parse, check
  * @param pathFile Path del file che si vuole parsare
  * @param configs Struct che si vuole modificare
  * @return 0 se non ci sono stati errori
  *         -1 se ci sono stati errori
  */
int initParseCheck(char * pathFile, config * configs);
/**
  * @function check
  *
  * @brief Funzione che controlla se i valori contenuti in una struct sono validi
  * @param configs Struct che si vuole controllare
  * @return 0 se non ci sono stati errori
  *         -1 se ci sono stati errori
  */
int check(config * configs);
/**
  * @function init
  *
  * @brief Funzione che inizializza i valori di una determinata structs
  * @param configs Struct che si vuole inizializzare
  */
void init(config * configs);
/**
  * @function FreeConfig
  *
  * @brief Funzionen che libera un'intera struct config
  * @param configs Struct che si vuole liberare
  */
void FreeConfig(config * configs);

#endif
