#include <stdio.h>
#include <stdlib.h>
#include "../../parser.h"

/*
 *
 */
int main(int argc, char** argv) {
    config conf;

    if(initParseCheck("../DATA/chatty.conf1", &conf) != 0)
      printf("ERRORE \n");

    printf("UnixPath: %s \n", conf.UnixPath);
    printf("MaxConnections: %d\n", conf.MaxConnections);
    free(conf.UnixPath); //TODO scrivere nella documentazione che bisogna fare la free di questi 3 campi
    free(conf.DirName);
    free(conf.StatFileName);

    return (EXIT_SUCCESS);
}
