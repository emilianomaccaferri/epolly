#include "h/handler.h"
#include "h/connection_context.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <strings.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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

void *handler_process_request(void* h){

    handler* current_handler = (handler *) h;
    char buf[current_handler->request_buffer_size];

    bzero(buf, current_handler->request_buffer_size);
    current_handler->request_buffer = buf; // let's keep the request buffer on the stack so it's faster to be read from

    while(current_handler->active){

        int ready_events = epoll_wait(current_handler->epoll_fd, current_handler->events, current_handler->max_events, -1);
        for(int i = 0; i < ready_events; i++){

            ssize_t received_bytes = recv(current_handler->events[i].data.fd, current_handler->request_buffer, current_handler->request_buffer_size, O_NONBLOCK);
            int request_size = 0;

            if(received_bytes > 0){

                connection_context* ctx = malloc(sizeof(connection_context));
                context_init(ctx, current_handler->request_buffer_size);

                ctx->fd = current_handler->events[i].data.fd;
                write_to_context(ctx, current_handler->request_buffer, received_bytes);

                /*
                    since we are in edge-triggered mode, we must consume the request at its 
                    fullest before exiting this loop.
                    to do so, we must read bytes until EAGAIN (or EWOULDBLOCK) isn't returned
                */
                while((received_bytes = recv(current_handler->events[i].data.fd, current_handler->request_buffer, current_handler->request_buffer_size, O_NONBLOCK)) > 0){

                    write_to_context(ctx, current_handler->request_buffer, received_bytes);
                    bzero(current_handler->request_buffer, current_handler->request_buffer_size);

                    request_size += received_bytes;
                    if(request_size > current_handler->max_request_size){
                        // do something to tell the client that the request is too big 
                        continue;
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
                    close(current_handler->events[i].data.fd);
                    epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, current_handler->events[i].data.fd, NULL);
                
                }
                
            }else if (received_bytes == 0){
                
                printf("client on descriptor %d disconnected\n", current_handler->events[i].data.fd);
                close(current_handler->events[i].data.fd);
                epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, current_handler->events[i].data.fd, NULL);

            }

        }

    }

    pthread_exit(0);

}

void handler_init(handler* handler, int max_events, int buf_size, int max_request_size){

    handler->thread = (pthread_t*) malloc(sizeof(pthread_t));
    handler->active = true;
    handler->max_events = max_events;
    handler->epoll_fd = epoll_create(max_events);
    handler->request_buffer_size = buf_size;
    handler->max_request_size = max_request_size;
    handler->events = malloc(sizeof(struct epoll_event) * handler->max_events);
    
    pthread_create(handler->thread, NULL, handler_process_request, (void*) handler);

}