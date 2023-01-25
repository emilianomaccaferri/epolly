#include "h/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

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

    unsigned int i = 0, line_len = 0;
    char** lines = (char**) malloc(sizeof(char*) * lines_num);
    char* copy = malloc(sizeof(char) * byte_len); // strtok_r destroys the string too :(
    char* temp;
    
    strncpy(copy, bytes, byte_len);
    
    char* part = strtok_r(copy, "\n", &temp);
    
    do{
        line_len = strlen(part);
        lines[i] = malloc(sizeof(char) * line_len);
        strncpy(lines[i], part, line_len);
        i++;
    }while ((part = strtok_r(NULL, "\n", &temp)) != NULL);

    return lines;

}

char* filename_to_mimetype_header(char* filename){

    char* file_extension = filename_to_extension(filename);

    // again, no hashmap, sorry...

    if(strcmp(".html", file_extension) == 0) 
        return "Content-Type: text/html; charset=utf-8\n";
    if(strcmp(".txt", file_extension) == 0)
        return "Content-Type: text/plain; charset=utf-8\n";
    if(strcmp(".jpg", file_extension) == 0)
        return "Content-Type: image/jpeg\n";
    if(strcmp(".jpeg", file_extension) == 0)
        return "Content-Type: image/jpeg\n";
    if(strcmp(".css", file_extension) == 0)
        return "Content-Type: text/css\n";
    
    return "Content-Type: application/octet-stream\n"; // the default for many webservers

}

char* filename_to_extension(char* filename){

    int len = strlen(filename), i = len - 1, end, ext_length = 0;
    char* ext;

    while(filename[i] != '.' && i >= 0){
        i--;
    }

    if(i == 0) return NULL;
    end = i;
    ext_length = len - end;
    ext = malloc(sizeof(char) * ext_length + 1);
    ext = strncpy(ext, filename + end, ext_length);
    ext[ext_length] = 0;
    
    return ext;

}

char* read_whole_file(int fd){

    char* string = malloc(sizeof(char) * 8092);
    char buffer[4096];
    int result;
    int total_bytes_read, current_bufferized_bytes;
    void* next_ptr;

    bzero(buffer, 4096);
    bzero(string, 8092);

    while((result = read(fd, buffer, 4096))){

        total_bytes_read += result;
        current_bufferized_bytes += result;
        if(current_bufferized_bytes >= 8092){
        
            next_ptr = (char*) realloc(string, total_bytes_read + 8092);
            if(!next_ptr){
                perror("system is out of memory!\n");
                exit(-1);
            }
            string = next_ptr;
            current_bufferized_bytes = 0;

        }

        strncat(string, buffer, result);

    }

    return string;

}