#ifndef PARSER_H_
#define PARSER_H_
#endif /* PARSER_H */

typedef struct config_s{
    char * UnixPath;
    char * MaxConnections;
    char * ThreadsInPool;
    char * MaxMsgSize;
    char * MaxFileSize;
    char * MaxHistMsgs;
    char * DirName;
    char * StatFileName;
} config;

/*
 * @function parse
 * 
 * @brief Prende un file e inserisce in configs tutte le configurazioni presenti nel file
 * @param nomeFile nome del file che si vuole parsare
 * @param configs array di configurazioni
 * 
 * @return 0 se non ci sono stati errori
 *         -1 se ci sono stati errori
 *  
 */
int parse(char * nomeFile, config * configs);


