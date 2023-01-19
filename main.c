#define MAX_EVTS 128
#define MAX_HEADERS_SIZE 1024 // 1Kb headers (max)
#define MAX_BODY_SIZE 2048 // 2Kb body (max)
#define MAX_REQUEST_SIZE (MAX_HEADERS_SIZE + MAX_BODY_SIZE)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include "lib/connection_context.h"
#include "lib/utils.h"
#include "lib/server.h"


void handle_request(connection_context* context){

    printf("handling descriptor #%d's request\n", context->fd);
    printf("the full request is: %s\n", context->data);

    if(context->length != 0){
        /* 
            length represents the length of the contents on the heap.
            if there is nothing allocated on the heap (dummy context passed if the request is shorter than the buffer size)
            then we won't be able to free it!
        */
        free(context->data);
        context->length = 0; 
    }

    char buf[100];

    int bytes_written = sprintf(buf, "hello, descriptor %d", context->fd);

    write(context->fd, buf, bytes_written);

}

void write_to_context(connection_context* context, char* data, ssize_t received_bytes){
    
    void* next_ptr;
    if(received_bytes > context->buf_size){
        perror("invalid buffer size!");
        exit(-1);
    }

    memcpy(context->data + context->length, data, received_bytes);
    context->length += received_bytes;
    if(context->length >= context->allocations * context->buf_size){
        next_ptr = (char*) realloc(context->data, context->length + context->buf_size);
        if(!next_ptr){
            perror("system is out of memory!\n");
            exit(-1);
        }
        context->data = next_ptr;
        context->allocations += 1;
    }

}


int main(void){

    int socket_fd,
        connection,
        active = 1;
    short port = 8080;
    char buffer[MAX_REQUEST_SIZE]; // buffer for the request headers
    connection_context* context = malloc(sizeof(connection_context) * MAX_EVTS); // one context for each connection
    server* http_server = server_init(port);

    printf("server is now listening on localhost:%d\n", port);

}