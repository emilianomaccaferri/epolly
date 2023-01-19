#pragma once
#include <stdbool.h>
#include <pthread.h>

/*
    the handler will process every request it gets from the main thread (i.e the server).
    every handler is thread that has a fixed-size epoll queue where connection events will occur; 
    for every event, the handler will parse the request and do something with it.

*/

typedef struct {
    
    /*
        every handler has a fixed-size epoll queue.
        if more than "max_events" file descriptors are ready when
        epoll_wait() is called, then successive epoll_wait() calls will
        round robin through the set of ready file descriptors. (https://man7.org/linux/man-pages/man2/epoll_wait.2.html)
    */
    int max_request_size;
    int max_events; 
    int epoll_fd;
    int request_buffer_size;
    char* request_buffer; // where the request will be written
    struct epoll_event* events;
    pthread_t* thread;
    bool active;

} handler;

void handler_init(handler* handler, int max_events, int buf_size, int max_request_size);