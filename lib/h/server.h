#pragma once
#include "handler.h"
#include <stdbool.h>

/*
    the server is the core of the program.
    this module will accept requests from connecting clients and will dispatch
    them to certain handlers.
*/

typedef struct {
    int socket_fd;
    int epoll_fd;
    short port;
    /*
        how many "parallel" file descriptors (i.e connections) the server will monitor.
        If more than "max_connection_events" file descriptors are ready when
        epoll_wait() is called, then successive epoll_wait() calls will
        round robin through the set of ready file descriptors. (https://man7.org/linux/man-pages/man2/epoll_wait.2.html)
    */
    int max_connection_events;
    int max_request_size;
    int num_handlers;
    int selector; // this will be the index of the last accessed handler
    struct epoll_event* connection_events;
    handler* handlers;
    bool active;
} server;

extern server* server_init(
    short port, 
    int max_events, 
    int num_handlers,
    int max_epoll_handler_queue_size,
    int request_buffer_size,
    int max_request_size
);
extern void server_loop(server* server);
extern void server_on_connection(server* server);
