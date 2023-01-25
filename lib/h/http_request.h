#pragma once
/*
    we'll only handle GET requests for simplicity.
    what we need is a way to parse simple requests (we will only parse the filename and the method)
*/
#include <stdio.h>
#include "connection_context.h"
typedef enum {
    GET
} http_method;

typedef struct {
    char* bytes;
    size_t length;
    int lines_num;
    char** lines;
    http_method method;
    char* filename;
    int filename_max_length;
    int filename_actual_length;
} http_request;

extern int http_request_create(http_request* req, connection_context* ctx);