#include "parser.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utility.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>


int initParseCheck(char * nomeFile, config * configs){
    int p;
    init(configs);
    if((p = parse(nomeFile, configs)) != 0)
      return p;
    return check(configs);
}

int check(config * configs){
    if(configs->UnixPath == NULL || strlen(configs->UnixPath) == 0) return -1;
    if(configs->MaxConnections <= 0) return -1;
    if(configs->ThreadsInPool <= 0) return -1;
    if(configs->MaxMsgSize <= 0) return -1;
    if(configs->MaxFileSize <= 0) return -1;
    if(configs->MaxHistMsgs <= 0) return -1;
    if(configs->DirName == NULL || strlen(configs->DirName) == 0) return -1;
    if(configs->StatFileName == NULL || strlen(configs->StatFileName) == 0) return -1;
    return 0;
}

void init(config * configs){
    configs->UnixPath = NULL;
    configs->MaxConnections = -1;
    configs->ThreadsInPool = -1;
    configs->MaxMsgSize = -1;
    configs->MaxFileSize = -1;
    configs->MaxHistMsgs = -1;
    configs->DirName = NULL;
    configs->StatFileName = NULL;
}

int parse(char * nomeFile, config * configs){
    FILE * fd;
    size_t len = 0;
    ssize_t read;
    char * line = NULL;
    char * save = NULL;
    int ok = 0;

    fd = fopen(nomeFile, "r");
    if(fd == NULL) {
        perror("Errore apertura file di configurazione");
        return errno;
    }
    errno = 0;
    while (((read = getline(&line, &len, fd)) != -1) && (ok == 0)) { //in line ci sarà la una riga del file conf
        RemoveSpaces(line);
        if(line[0] != '#' && strlen(line) != 0){ //è un commento o una riga vuota, li ignoro
            char * parametro = strtok_r(line, "=", &save);
            char * value = strdup(save);
            //printf("Parametro: |%s|\n", parametro);
            //printf("Value: |%s|\n", value);
            ok = makeConfig(parametro, value, configs);
            //free(parametro); //TODO perchè non funziona se metto le free?
            free(value);
        }
        errno = 0;
    }
    free(line);
    fclose(fd);
    if(ok != 0) return ok; // se c'è stato un errore nella makeConfig ritorno
    return errno; //se non ci sono stati errori, torna 0, altrimenti il codice di errore
}

//Se ci sono due parametri uguali nello stesso file di configurazione, verrà preso il valore dell'ultimo
int makeConfig(char * parametro, char * value, config * configs){
    if(strcmp(parametro, "UnixPath") == 0)
        configs->UnixPath = strdup(value);
    else if(strcmp(parametro, "MaxConnections") == 0)
        configs->MaxConnections = atoi(value); // atoi con stringa vuota ritorna 0
    else if(strcmp(parametro, "ThreadsInPool") == 0)
        configs->ThreadsInPool = atoi(value);
    else if(strcmp(parametro, "MaxMsgSize") == 0)
        configs->MaxMsgSize = atoi(value);
    else if(strcmp(parametro, "MaxFileSize") == 0)
        configs->MaxFileSize = atoi(value);
    else if(strcmp(parametro, "MaxHistMsgs") == 0)
        configs->MaxHistMsgs = atoi(value);
    else if(strcmp(parametro, "DirName") == 0)
        configs->DirName = strdup(value);
    else if(strcmp(parametro, "StatFileName") == 0)
        configs->StatFileName = strdup(value);
    else return -1;
    return 0;
}

// Funzione che elimina gli tutti gli spazi, i tab e gli \n dalla stringa source
void RemoveSpaces(char* source) {
    char* i = source;
    char* j = source;
    while(*j != 0) {
        *i = *j++;
        if((*i != ' ') && (*i != '\t') && (*i != '\n'))
            i++;
    }
    *i = 0;
}
