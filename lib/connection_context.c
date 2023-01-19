#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "h/connection_context.h"

void context_init(connection_context* context, unsigned int request_size){

    context->data = malloc(sizeof(char) * request_size);
    context->length = 0; // we didn't receive anything yet, so the size is 0
    context->buf_size = request_size; // defaults to this
    context->allocations = 1;

    bzero(context->data, request_size);

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