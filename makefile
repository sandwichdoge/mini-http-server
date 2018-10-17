CC = gcc
CFLAGS = -lpthread -lssl -lcrypto

current_dir = $(shell pwd)
conf_demo_dir := PATH=$(current_dir)
conf_demo_dir := $(conf_demo_dir)/demo-site-1
conf_port_http := PORT=80
conf_port_ssl := PORT_SSL=443
conf_home := HOME=/index.html
conf_ssl_cert_file_pem := SSL_CERT_FILE_PEM=$(current_dir)/certificate.pem
conf_ssl_key_file_pem := SSL_KEY_FILE_PEM=$(current_dir)/key.pem


all: http_server.o socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o
	$(CC) -g http_server.o socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o $(CFLAGS) -o start-server
	@echo GENERATING CONFIGS..
	@echo DO NOT COMMENT ON THE SAME LINE AS PARAMETERS > http.conf
	@echo $(conf_demo_dir) >> http.conf
	@echo $(conf_port_http) >> http.conf
	@echo $(conf_port_ssl) >> http.conf
	@echo $(conf_home) >> http.conf
	@echo $(conf_ssl_cert_file_pem) >> http.conf
	@echo $(conf_ssl_key_file_pem) >> http.conf


http_server.o: http_server.c socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o casing.h str-utils.h sysout.h
	$(CC) -c -g http_server.c -o http_server.o

http-request.o: http-request.c http-request.h
	$(CC) -c http-request.c

fileops.o: fileops.c fileops.h
	$(CC) -c fileops.c

mime/http-mimes.o: mime/http-mimes.c mime/http-mimes.h
	$(CC) -c mime/http-mimes.c -o mime/http-mimes.o

serversocket.o: socket/serversocket.c socket/serversocket.h
	$(CC) -c socket/serversocket.c

http-ssl.o: socket/http-ssl.c socket/http-ssl.h
	$(CC) -c socket/http-ssl.c


ssl-ca:
	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out certificate.pem
	openssl x509 -text -noout -in certificate.pem


clean:
	rm socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o http_server.o