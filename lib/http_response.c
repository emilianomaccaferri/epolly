#include "h/http_response.h"
#include <string.h>
#include <stdlib.h>

http_response* http_response_create(int status, char* headers, char* body){
    /*
        status: the HTTP status
        body: a null-terminated string representing the response body
        headers: a null-terminated string representing the response headers
    */

    http_response* res = (http_response*) malloc(sizeof(http_response));
    res->content_length = strlen(body);
    res->headers_length = strlen(headers);
    res->response_length = res->content_length + res->headers_length;
    res->headers = malloc(sizeof(char) * res->headers_length);
    res->body = malloc(sizeof(char) * res->content_length);
    res->status = status;

    strcpy(res->headers, headers);
    strcpy(res->body, body);

    return res;

}

char* http_response_stringify(http_response* res){

    char* response_string = malloc(sizeof(char) * res->response_length);
    strcat(response_string, res->headers);
    strcat(response_string, "\n\r\n");
    strcat(response_string, res->body);
    response_string[res->response_length] = '\0'; // 0-terminate the response

    return response_string;

}