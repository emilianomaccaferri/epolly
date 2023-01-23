#pragma once

typedef struct {
    int status;
    int content_length;
    int headers_length;
    int response_length;
    char* body;
    char* headers;
} http_response;

extern http_response* http_response_create(int status, char* headers, char* body);
extern char* http_response_stringify(http_response* res);