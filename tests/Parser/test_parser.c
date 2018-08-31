/** @file test_parser.c
  * @author Lorenzo Gazzella 546890
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */

#include <stdio.h>
#include <stdlib.h>
#include "../../parser.h"

/*
 *
 */
int main(int argc, char** argv) {
    config conf;

    if(initParseCheck("../../DATA/chatty.conf1", &conf) != 0)
      printf("ERRORE \n");

    printf("UnixPath: %s \n", conf.UnixPath);
    printf("MaxConnections: %d\n", conf.MaxConnections);
    FreeConfig(&conf);


    return (EXIT_SUCCESS);
}
