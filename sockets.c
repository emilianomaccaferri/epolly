#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_EVTS 128
#define MAX_HEADERS_SIZE 1024 // 1Kb headers (max)
#define MAX_BODY_SIZE 2048 // 2Kb body (max)
#define MAX_REQUEST_SIZE (MAX_HEADERS_SIZE + MAX_BODY_SIZE)

typedef struct {
    int length; // as an example, max message size is 4GB (i won't check for overflows because i'm lazy and this is an example)
    char* data;
    int allocations;
    int buf_size; // defaults to this
    int fd;
} connection_context;

void handle_request(connection_context* context){

    printf("handling descriptor #%d's request\n", context->fd);
    printf("the full request is: %s\n", context->data);

    if(context->length != 0){
        /* 
            length represents the length of the contents on the heap.
            if there is nothing allocated on the heap (dummy context passed if the request is shorter than the buffer size)
            then we won't be able to free it!
        */
        free(context->data);
        context->length = 0; 
    }

    char buf[100];

    int bytes_written = sprintf(buf, "hello, descriptor %d", context->fd);

    write(context->fd, buf, bytes_written);

}
void init_context(connection_context* context){

    context->data = malloc(sizeof(char) * MAX_REQUEST_SIZE);
    context->length = 0; // we didn't receive anything yet, so the size is 0
    context->buf_size = MAX_REQUEST_SIZE; // defaults to this
    context->allocations = 1;

    bzero(context->data, MAX_REQUEST_SIZE);

}
void write_to_context(connection_context* context, char* data, ssize_t received_bytes){
    
    void* next_ptr;
    if(received_bytes > context->buf_size){
        perror("invalid buffer size!");
        exit(-1);
    }

    memcpy(context->data + context->length, data, received_bytes);
    context->length += received_bytes;
    if(context->length >= context->allocations * context->buf_size){
        next_ptr = (char*) realloc(context->data, context->length + context->buf_size);
        if(!next_ptr){
            perror("system is out of memory!");
            exit(-1);
        }
        context->data = next_ptr;
        context->allocations += 1;
    }

}

void make_nonblocking(int fd){

    int flags = fcntl(fd, F_GETFL, 0); // manipulate file descriptor with some flags
    if(flags == -1){
        perror("cannot get file descriptor's flags\n");
        exit(-1);
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        perror("cannot set socket to non-blocking\n");
        exit(-1);
    }

}

int create_socket(short port){

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int socket_opt = 1;
    int socket_flags;
    struct sockaddr_in server_address, client_address;

    if(socket_fd < 0){
        perror("error while opening socket\n");
        exit(-1);
    } 

    // avoid EADDRINUSE

    int opt = 1;
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

int main(void){

    int socket_fd,
        connection,
        active = 1;
    short port = 8080;
    char buffer[MAX_REQUEST_SIZE]; // buffer for the request headers
    connection_context* context = malloc(sizeof(connection_context) * MAX_EVTS); // one context for each connection

    /*
        this creates a file descriptor that serves as an endpoint for communication; 
        in particular, this is an IPv4 socket (AF_INET) for TCP (SOCK_STREAM) connections.

        since this socket will support a single protocol, the "protocol" argument is set to 0.        
        
    */
    socket_fd = create_socket(port);
   
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    if(epoll_fd < 0){
        perror("cannot create epoll\n");
        exit(-1);
    }

    struct epoll_event on_socket_conn;
    struct epoll_event events[MAX_EVTS];

    on_socket_conn.events = EPOLLIN; // only poll input events (such as new connections!)
    on_socket_conn.data.fd = socket_fd; // poll only on the socket descriptor!

    // add an epoll to the kernel
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &on_socket_conn) < 0){

        printf("cannot add epoll on socket\n");
        exit(-1);

    }

    printf("server is now listening on localhost:%d\n", port);

    while(active){

        int received_events = epoll_wait(epoll_fd, events, MAX_EVTS, -1); // infinitely wait for I/O events on the monitored descriptor (the socket!)
        for(int i = 0; i < received_events; i++){

            if(events[i].data.fd == socket_fd){

                /* 
                    a new connection is being notified!
                    i should handle it!
                */
                struct sockaddr_in client_in;
                int client_len = sizeof(client_in);
                int client_fd = accept(socket_fd, (struct sockaddr *) &client_in, &client_len);
                if(client_fd < 0){
                    if(errno != EAGAIN || errno != EWOULDBLOCK){
                        perror("error while accepting\n");
                        exit(-1);
                    }
                }

                /* 
                    let's make a new epoll, only that, this time, we are going to monitor the client
                    that just connected!
                    what we need to do is:
                        - to allocate the context for its request
                        - to create a new epoll
                */
                
                make_nonblocking(client_fd);

                struct epoll_event client_event;
                connection_context* ctx = malloc(sizeof(connection_context));
                init_context(ctx);
                ctx->fd = client_fd;
                
                client_event.events = EPOLLIN | EPOLLET; // edge-triggered mode for the current descriptor (EAGAIN)
                client_event.data.fd = client_fd;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0){ // adding a new epoll (EPOLL_CTL_ADD)
                    perror("cannot add client descriptor to epoll");
                    exit(-1);
                }
                                
                // thanks for connecting!

            }else{

                /*
                    someone else (a connected client) is notifying something!
                    i should parse what they are writing!
                */

                int request_size = 0;
                bzero(&buffer, MAX_REQUEST_SIZE);
                ssize_t received_bytes = recv(events[i].data.fd, &buffer, sizeof(buffer), O_NONBLOCK);

                connection_context* ctx = events[i].data.ptr;
                /*

                    what happens if the message is longer than BUF_SIZE?
                    we can use a dynamically allocated string that we will store on the heap.
                    this string will contain all the stuff we receive chunk-by-chunk from our recv():
                    all we need to do is to concatenate every chunk we receive into this string!

                    we can store this information on a dedicated array called "context", which will contain
                    an element for every descriptor mapped

                */

                if(received_bytes > 0){

                    /*
                        since we are in edge-triggered mode, we must consume the request at its 
                        fullest before exiting this loop.
                        to do so, we must read bytes until EAGAIN (or EWOULDBLOCK) isn't returned
                    */
                    write_to_context(ctx, buffer, received_bytes);

                    while((received_bytes = recv(events[i].data.fd, &buffer, sizeof(buffer), O_NONBLOCK)) > 0){
                        write_to_context(ctx, buffer, received_bytes);
                        bzero(&buffer, MAX_REQUEST_SIZE);
                        request_size += received_bytes;
                        if(request_size > MAX_REQUEST_SIZE){
                            // do something to tell the client that the request is too big 
                            break;
                        }
                    }

                    /*
                        the while has ended so we stopped reading.
                        we could've encountered an error, so we must check for it!
                    */

                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        // we drained the descriptor
                        handle_request(ctx);
                    }else{

                        /*
                            something bad happened to our client, let's ignore its request
                        */
                        close(events[i].data.fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    
                    }
                    
                }else if (received_bytes == 0){
                    
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);

                }

            }

        }

    }

}