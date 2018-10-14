all: http_server.o serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o
	gcc -g http_server.o serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o -lpthread -lssl -lcrypto -pthread -o start-server

http_server.o: http_server.c serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o casing.h str-utils.h sysout.h
	gcc -c -g http_server.c -o http_server.o

serversocket.o: serversocket.c serversocket.h
	gcc -c serversocket.c

http-request.o: http-request.c http-request.h
	gcc -c http-request.c

fileops.o: fileops.c fileops.h
	gcc -c fileops.c

http-mimes.o: http-mimes.c http-mimes.h
	gcc -c http-mimes.c

http-ssl.o: http-ssl.c http-ssl.h
	gcc -c http-ssl.c

clean:
	rm serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o http_server.o