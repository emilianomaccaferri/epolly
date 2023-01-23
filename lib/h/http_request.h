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
} http_request;

extern http_request* http_request_create(connection_context* ctx);