#include "h/http_request.h"
#include "h/connection_context.h"
#include "h/utils.h"
#include <stdlib.h>
#include <string.h>

http_request* http_request_create(connection_context* ctx){

    char* bytes = malloc(sizeof(char) * ctx->length);
    http_request* req = malloc(sizeof(http_request));
    req->bytes = malloc(sizeof(char) * ctx->length);
    memcpy(req->bytes, ctx->data, ctx->length); // copy the request so we can free the context data later
    req->length = ctx->length;
    req->lines_num = count_lines(req->bytes, req->length);
    req->lines = bytes_to_lines(req->bytes, req->length, req->lines_num);
    
    return req;

}