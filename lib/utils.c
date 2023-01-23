#include "h/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

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

int count_lines(char* bytes, size_t byte_len){

    char* copy = malloc(sizeof(char) * byte_len); // strch destroys the string, so we make a copy of it
    char* next;
    int count = 0;

    copy = memcpy(copy, bytes, byte_len);
    next = strchr(copy, '\n');

    while(next != NULL){
        count++;
        next = strchr(next + 1, '\n');
    }

    free(copy);
    return count;

}

char** bytes_to_lines(char* bytes, size_t byte_len, int lines_num){

    unsigned int i = 0;
    char** lines = (char**) malloc(sizeof(char*) * lines_num);
    char* copy = malloc(sizeof(char) * byte_len); // strtok_r destroys the string too :(
    char* temp;
    
    strcpy(copy, bytes);
    
    char* part = strtok_r(copy, "\n", &temp);
    
    do{
        lines[i] = malloc(sizeof(char) * strlen(part));
        strcpy(lines[i], part);
        i++;
    }while ((part = strtok_r(NULL, "\n", &temp)) != NULL);

    return lines;

}