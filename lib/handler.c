#include "h/handler.h"
#include "h/connection_context.h"
#include "h/http_request.h"
#include "h/http_response.h"
#include "h/utils.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <strings.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

http_response* build_response(connection_context* context){

    printf("handling descriptor #%d's request\n", context->fd);

    http_request* req = malloc(sizeof(http_request));
    http_response* res;

    int req_err = http_request_create(req, context);

    if(context->length != 0){
        /* 
            length represents the length of the contents on the heap.
            if there is nothing allocated on the heap (dummy context passed if the request is shorter than the buffer size)
            then we won't be able to free it!
        */
        free(context->data);
        context->length = 0; 
    }

    if(req_err != 0){
        printf("error while replying\n");
        switch(errno){
            case -1:
                res = http_response_bad_request(context->fd);
            break;
            case 1:
                // method is unimplemented (we only have GET)
                res = http_response_uninmplemented_method(context->fd);
            break;
            case 2: 
                // filename too long
                res = http_response_filename_too_long(context->fd);
            break;
            default: res = http_response_internal_server_error(context->fd);
        }
    }else{
        
        int fd = open(req->filename, O_RDONLY);
        if(fd < 0){
            res = http_response_not_found(context->fd);
        }else{
            char* mime_type = filename_to_mimetype_header(req->filename);
            char* stringified_file = read_whole_file(fd);
            res = http_response_create(200, mime_type, stringified_file, context->fd);
        }

    }

    /*
        we are ready to stream the response to the socket! 
        to actually stream something we need to save the socket's fd (check comment 1 in h/http_response.h)
        so that when we are back in the epoll loop we have a valid file descriptor to stream to 
        (we can't use both the "fd" and the "ptr" attributes when using epolls)
    */

    return res;

}

void *handler_process_request(void* h){

    handler* current_handler = (handler *) h;
    char buf[current_handler->request_buffer_size];

    bzero(buf, current_handler->request_buffer_size);
    current_handler->request_buffer = buf; // let's keep the request buffer on the stack so it's faster to be read from

    while(current_handler->active){

        int ready_events = epoll_wait(current_handler->epoll_fd, current_handler->events, current_handler->max_events, -1);
        for(int i = 0; i < ready_events; i++){

            uint32_t events = current_handler->events[i].events;

            if(events == EPOLLIN){
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
                    while((received_bytes = recv(ctx->fd, current_handler->request_buffer, current_handler->request_buffer_size, O_NONBLOCK)) > 0){

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
                        /*
                            we drained the descriptor (basically, we've read the whole request)
                            now we have to actually reply to the request, so we set the event descriptor
                            ready for writing (we attach the EPOLLOUT bit).

                            note that we removed the EPOLLET flag, because we don't want to
                            write the whole response for each request. 
                            what we need is a way to write a non-blocking response, so we can write
                            multiple responses concurrently.
                        */
                        
                        http_response* res = build_response(ctx);

                        struct epoll_event add_write_event;
                        add_write_event.events = EPOLLOUT; 
                        add_write_event.data.ptr = res;

                        /*
                            on the same file descriptor we can attach everything, we can also listen to changes on a certain
                            data structure! 
                            that's what we did with the "ptr" attribute of the epoll_event: 
                            we put our response there so, when there is data being written on that data structure, the epoll will trigger
                            a new event!
                            we will write data as soon as we process the request, so the epoll will trigger instantly.

                            this method makes us able to write the response chunk by chunk (check (**) down below), without blocking other requests in the loop!
                        */

                        if(epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_MOD, ctx->fd, &add_write_event) < 0){
                            perror("cannot set epoll descriptor ready for writing\n");
                            exit(-1);
                        }

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
            }else if(events == EPOLLOUT){

                /*
                    (**) here we are, consuming what we wrote (an http response) on the epoll! 
                    this part of the loop enables us, as already mentioned, to stream the http response to the client without blocking 
                    other input or output streams!
                    with the use of the "stream_ptr" variable (check lib/h/http_response.h, comment 2) we can efficiently keep track
                    of the chunk we are writing to the client, incrementing it whenever we write some bytes!

                    note how we write data: we use the streamed_response->stringified buffer + the stream_ptr (basically an offset), 
                    because in this way we keep track of what we already wrote (by summing stream_ptr).
                */

                http_response* streamed_response = (http_response*) current_handler->events[i].data.ptr;
                int written_bytes = send(
                    streamed_response->socket, 
                    streamed_response->stringified + streamed_response->stream_ptr, 
                    streamed_response->full_length,
                    0
                );
                if(written_bytes >= 0){
                    streamed_response->stream_ptr += written_bytes; // here we update our pointer!
                    if(streamed_response->stream_ptr == streamed_response->full_length){
                        /*
                            we wrote everything, so we need to set the epoll back to EPOLLIN, 
                            so that we can read more requests from the socket!
                        */

                        struct epoll_event back_to_reading;
                        back_to_reading.data.fd = streamed_response->socket;
                        back_to_reading.events = EPOLLIN | EPOLLET; // edge triggered again
                        if(epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_MOD, streamed_response->socket, &back_to_reading) < 0){
                            perror("cannot reset socket to read state");
                        }
                        close(back_to_reading.data.fd);

                    }
                }else if(written_bytes == -1){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        // the socket is not available yet
                        continue;
                    }else{
                        perror("send failed");
                    }
                }

            }else{
                /*
                    something bad happened to our client, let's ignore its request
                */
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