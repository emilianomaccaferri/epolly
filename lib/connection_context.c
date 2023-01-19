#include "connection_context.h"

void context_init(connection_context* context, ssize_t request_size){

    context->data = malloc(sizeof(char) * request_size);
    context->length = 0; // we didn't receive anything yet, so the size is 0
    context->buf_size = request_size; // defaults to this
    context->allocations = 1;

    bzero(context->data, request_size);

}