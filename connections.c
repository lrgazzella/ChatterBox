#include <connections.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> 

int openConnection(char* path, unsigned int ntimes, unsigned int secs){
    int fd_skt = socket(AF_UNIX, SOCK_STREAM, 0); // creo la socket
    
    struct sockaddr_un sa;      
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX; 
    
    struct timespec req = {secs, 0}; //TODO carfi
    
    int i = 0;
    for(i=0; i<ntimes; i++){
        if(connect(fd_skt, (struct sockaddr*) &sa, sizeof(sa)) != -1) 
            return fd_skt;
        else {
            nanosleep(&req, (struct timespec *) NULL);
        }
    }
    return -1;
}