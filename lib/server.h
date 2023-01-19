#pragma once
#include <stdbool.h>

typedef struct {
    int socket_fd;
    int epoll_fd;
    short port;
    int max_events;
    int max_request_size;
    struct epoll_event* events;
    bool active;
} server;

extern server* server_init(short port);
extern void server_loop(server* server);