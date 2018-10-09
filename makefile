all: http_server.o serversocket.o http-request.o fileops.o http-mimes.o
	gcc http_server.o serversocket.o http-request.o fileops.o http-mimes.o -lpthread -o start-server

http_server.o: http_server.c serversocket.o http-request.o fileops.o http-mimes.o casing.h strinsert.h sysout.h
	gcc -c http_server.c -o http_server.o

serversocket.o: serversocket.c serversocket.h
	gcc -c serversocket.c

http-request.o: http-request.c http-request.h
	gcc -c http-request.c

fileops.o: fileops.c fileops.h
	gcc -c fileops.c

http-mimes.o: http-mimes.c http-mimes.h
	gcc -c http-mimes.c

clean:
	rm serversocket.o http-request.o fileops.o http-mimes.o http_server.o