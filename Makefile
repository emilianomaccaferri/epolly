all: server
server:
	mkdir bin
	gcc main.c -o bin/server