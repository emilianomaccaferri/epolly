all: server server_debug
server:
	mkdir -p bin
	gcc -Wall -Wextra -c main.c lib/connection_context.c lib/handler.c lib/server.c lib/utils.c
	gcc -g *.o -o bin/server

clean:
	rm *.o
	rm bin/server