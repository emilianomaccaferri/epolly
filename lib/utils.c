#include <fcntl.h>
#include "utils.h"

void make_nonblocking(int fd){

    int flags = fcntl(fd, F_GETFL, 0); // manipulate file descriptor with some flags
    if(flags == -1){
        perror("cannot get file descriptor's flags\n");
        exit(-1);
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        perror("cannot set socket to non-blocking\n");
        exit(-1);
    }

}