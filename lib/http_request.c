#include "h/http_request.h"
#include "h/connection_context.h"
#include "h/utils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define FILENAME_MAX_LEN 1024
#define WWW_PATH "/home/macca/stuff/epolly/www"

static int http_request_parse_method(http_request* req);
static int http_request_parse_filename(http_request* req);
int http_request_create(http_request* req, connection_context* ctx){

    /*
        logic that parses an http request 
        returns NULL (malformed request) if:
            - request is empty or has no lines
            - request has an invalid method

        return the request but with an errno attached if:
            - the method is unimplemented (we only have GET lol)
    */

    if(ctx->length == 0) return -1;
    
    req->bytes = malloc(sizeof(char) * ctx->length + 1);
    memcpy(req->bytes, ctx->data, ctx->length + 1); // copy the request so we can free the context data later
    req->length = ctx->length;
    req->bytes[req->length + 1] = '\0'; // terminate the string
    req->lines_num = count_lines(req->bytes, req->length);
    req->filename_max_length = FILENAME_MAX_LEN;

    if(req->lines_num == 0){
        free(req);
        return -1;
    }

    req->lines = bytes_to_lines(req->bytes, req->length, req->lines_num);
    
    if(http_request_parse_method(req) < 0){
        errno = 1;
    }
    if(http_request_parse_filename(req) < 0){
        errno = 2;
    }
    return 0;

}

int http_request_parse_filename(http_request* req){

    int i = 0, 
        www_path_len = strlen(WWW_PATH), 
        k = www_path_len;

    char* first_line = req->lines[0];

    char filename[www_path_len + req->filename_max_length];
    bzero(filename, www_path_len + req->filename_max_length);
    strncpy(filename, WWW_PATH, www_path_len);

    while(first_line[i] != '/'){
        i++;
    } // we searched for the /, now we can start reading the filename

    while(first_line[i] != ' '){
        if(i == req->filename_max_length){
            return -1;
        }
        filename[k] = first_line[i];
        i++;
        k++;
    }
    
    req->filename = malloc(sizeof(char) * www_path_len + k);
    strncpy(req->filename, filename, www_path_len + k);
    req->filename_actual_length = www_path_len + k;
    return 0;

}

int http_request_parse_method(http_request* req){
    
    int line_len = strlen(req->lines[0]);
    char* first_line = malloc(sizeof(char) * line_len); // again, strtok_r destroys the string
    char* method, *temp;

    strncpy(first_line, req->lines[0], line_len);
    method = strtok_r(first_line, " ", &temp);

    if(strcmp("GET", method) == 0){
        req->method = GET;
        return 0;
    }

    return -1;

}