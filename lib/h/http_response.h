#pragma once

typedef struct {
    int status;
    int content_length;
    int headers_length;
    int response_length;
    int full_length;
    /*
        (1. )
        we need to save the socket inside this struct, because otherwise we will
        lose the file descriptor once we attach a response to an epoll
    */
    int socket;
    char* body;
    char* headers;
    char* stringified;
    /*
        (2. )
        the stream_ptr variable will be useful when the request will be streamed to the client.
        with this variable will act as a "file pointer" to our stream, because
        it will be incremented for each byte written to the client, so we know
        where we left off!
    */
    int stream_ptr;
} http_response;

extern http_response* http_response_create(int status, char* headers, char* body, int socket_fd);
extern http_response* http_response_bad_request(int socket_fd);
extern http_response* http_response_uninmplemented_method(int socket_fd);
extern char* http_response_stringify(http_response* res);