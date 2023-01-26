#include "h/server.h"
#include "h/handler.h"
#include "h/utils.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

static int create_socket(short port);

static int create_socket(short port){

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int socket_opt = 1;
    struct sockaddr_in server_address;

    if(socket_fd < 0){
        perror("error while opening socket\n");
        exit(-1);
    } 

    // avoid spurious EADDRINUSE
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0) {
        perror("error while setting socket options\n");
        exit(-1); 
    }

    bzero((struct sockaddr_in *) &server_address, sizeof(server_address)); // clear server_address' struct
    server_address.sin_family = AF_INET; // IPv4 
    server_address.sin_addr.s_addr = INADDR_ANY; // mapped on 0.0.0.0, listening on every interface
    server_address.sin_port = htons(port); // HTTP port;

    /* 
        note: the casting from "sockaddr_in" to "sockaddr" is made because sockaddr is a generic address structure,
        whereas sockaddr_in is a specific one (used for INTERNET communications - "_in" stands for that!).
        the functions "bind" and "accept" are generic, so they accept a sockaddr struct! 
    */

    if(bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
        perror("error while binding server's port\n");
        exit(-1);
    }

    // listen for connection on port 80 for a maximum of 128 queued connections at a time

    if(listen(socket_fd, 128) < 0){
        perror("error while listening\n");
        exit(-1);
    }

    make_nonblocking(socket_fd);

    return socket_fd;

}

server* server_init(
        short port, 
        int max_events, 
        int num_handlers,
        int max_epoll_handler_queue_size,
        int request_buffer_size,
        int max_request_size
    ){

    /*
        this creates a file descriptor that serves as an endpoint for communication; 
        in particular, this is an IPv4 socket (AF_INET) for TCP (SOCK_STREAM) connections.

        since this socket will support a single protocol, the "protocol" argument is set to 0.        
        
    */
    server* http_server = (server *) malloc(sizeof(server));

    http_server->max_connection_events = max_events;
    http_server->socket_fd = create_socket(port);
    http_server->epoll_fd = epoll_create1(0);
    http_server->connection_events = malloc(sizeof(struct epoll_event) * http_server->max_connection_events);
    http_server->active = false;
    http_server->num_handlers = num_handlers;
    http_server->selector = 0;
    http_server->handlers = (handler *) malloc(sizeof(handler) * http_server->num_handlers);

    if(http_server->epoll_fd < 0){
        perror("cannot create epoll\n");
        exit(-1);
    }

    struct epoll_event on_socket_conn;

    on_socket_conn.events = EPOLLIN; // only poll input events (such as new connections!)
    on_socket_conn.data.fd = http_server->socket_fd; // poll only on the socket descriptor!

    // add an epoll to the kernel
    if(epoll_ctl(http_server->epoll_fd, EPOLL_CTL_ADD, http_server->socket_fd, &on_socket_conn) < 0){

        perror("cannot add epoll on socket\n");
        exit(-1);

    }

    /* initialize handlers */
    for(int i = 0; i < http_server->num_handlers; i++){
        handler_init(&http_server->handlers[i], max_epoll_handler_queue_size, request_buffer_size, max_request_size);
    }

    return http_server;

}

void server_on_connection(server* server){

    /* 
        a new connection is being notified!
        i should handle it!
    */
    struct sockaddr_in client_in;
    int client_len = sizeof(client_in);
    
    int client_fd = accept(server->socket_fd, (struct sockaddr *) &client_in, (socklen_t *) &client_len);

    if(client_fd < 0){
        if(errno != EAGAIN || errno != EWOULDBLOCK){
            perror("error while accepting\n");
            exit(-1);
        }
    }

    /* 
        let's make a new epoll, only that, this time, we are going to monitor the client
        that just connected!
    */
    
    /*
        client descriptor *must* be non-blocking because
        we need to wait for EAGAIN (or EWOULDBLOCK) to drain
        the request (and of course we need non-blocking io). 
    */
    make_nonblocking(client_fd);

    struct epoll_event client_event;
    
    client_event.events = EPOLLIN | EPOLLET; // edge-triggered mode for the current descriptor (EAGAIN)
    client_event.data.fd = client_fd;

    handler selected_handler = server->handlers[server->selector];
    server->selector = (server->selector + 1) % server->num_handlers;

    if(epoll_ctl(selected_handler.epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0){ // adding a new epoll (EPOLL_CTL_ADD)
        perror("cannot add client descriptor to epoll");
        exit(-1);
    }

    
                    
    // thanks for connecting!

}

void server_loop(server* server){

    server->active = true;

    while(server->active){

        // infinitely wait for I/O events on the monitored descriptor (the socket!)
        int received_events = epoll_wait(server->epoll_fd, server->connection_events, server->max_connection_events, -1); 

        for(int i = 0; i < received_events; i++){

            if(server->connection_events[i].data.fd == server->socket_fd){

                server_on_connection(server);

            }

        }

    }
}