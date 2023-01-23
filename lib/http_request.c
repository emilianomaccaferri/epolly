#include "h/http_request.h"
#include "h/connection_context.h"
#include "h/utils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int http_request_parse_method(http_request* req);
http_request* http_request_create(connection_context* ctx){

    /*
        logic that parses an http request 
        returns NULL (malformed request) if:
            - request is empty or has no lines
            - request has an invalid method

        return the request but with an errno attached if:
            - the method is unimplemented (we only have GET lol)
    */

   if(ctx->length == 0) return NULL;

    http_request* req = malloc(sizeof(http_request));
    req->bytes = malloc(sizeof(char) * ctx->length + 1);
    memcpy(req->bytes, ctx->data, ctx->length + 1); // copy the request so we can free the context data later
    req->length = ctx->length;
    req->bytes[req->length + 1] = '\0'; // terminate the string
    req->lines_num = count_lines(req->bytes, req->length);

    if(req->lines_num == 0){
        free(req);
        return NULL;
    }

    req->lines = bytes_to_lines(req->bytes, req->length, req->lines_num);
    
    if(http_request_parse_method(req) < 0){
        errno = 1;
    }

    return req;

}

int http_request_parse_method(http_request* req){

    char* first_line = malloc(sizeof(char) * strlen(req->lines[0])); // again, strtok_r destroys the string
    char* method, *temp;

    strcpy(first_line, req->lines[0]);
    method = strtok_r(first_line, " ", &temp);

    if(strcmp("GET", method) == 0){
        req->method = GET;
        return 0;
    }

    return -1;

}