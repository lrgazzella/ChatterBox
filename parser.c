#include <parser.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility.h>
#include <errno.h>

void makeConfig(char * parametro, char * value, config * configs);

int parse(char * nomeFile, config * configs, int * dim){
    int i = 0, fd;
    config * configs = NULL;
    size_t len = 0;
    ssize_t read;
    char * line = NULL;
    char * save = NULL;
    
    ec_meno1_return((fd = open(nomeFile, O_RDONLY)), "Errore apertura file di configurazione");
    errno = 0;
    while ((read = getline(&line, &len, fd)) != -1) { //in line ci sarà la una riga del file conf
        RemoveSpaces(line);
        if(line[0] == '#'){ //è un commento, lo ignoro
            free(line);
            line = NULL;
            len = 0;
        }else{
            char * parametro = strtok_r(line, "=", &save);
            char * value = strdup(save);
            
            makeConfig(parametro, value, configs);
            free(line); // TODO è corretto?
            line = NULL;
            free(parametro);
            free(value);
            len = 0;
        }
        errno = 0;
    }
    return errno; //se non ci sono stati errori, torna 0, altrimenti il codice di errore
}
void makeConfig(char * parametro, char * value, config * configs){
    switch(parametro){
        case "UnixPath":
            configs->UnixPath = strdup(value);
        break;
        case "MaxConnections":
            configs->MaxConnections = strdup(value);
        break;
        case "ThreadsInPool":
            configs->ThreadsInPool = strdup(value);
        break;
        case "MaxMsgSize":
            configs->MaxMsgSize = strdup(value);
        break;
        case "MaxFileSize":
            configs->MaxFileSize = strdup(value);
        break;
        case "MaxHistMsgs":
            configs->MaxHistMsgs = strdup(value);
        break;
        case "DirName":
            configs->DirName = strdup(value);
        break;
        case "StatFileName":
            configs->StatFileName = strdup(value);
        break;
    }
    //free(value);
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