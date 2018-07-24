#include <stdio.h>
#include <stdlib.h>
#include <parser.h>

/*
 * 
 */
int main(int argc, char** argv) {
    config conf;
    parse("../DATA/chatty.conf1", &conf);
    
    printf("UnixPath: %s", conf.UnixPath);
    printf("MaxConnections: %s", conf.MaxConnections);
    
    return (EXIT_SUCCESS);
}

