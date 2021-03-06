CC = gcc
CFLAGS = -Wall -O2 -march=native

current_dir = $(shell pwd)
conf_demo_dir := PATH=$(current_dir)
conf_demo_dir := $(conf_demo_dir)/demo-site-1
conf_port_http := PORT=80
conf_port_ssl := PORT_SSL=443
conf_home := HOME=/index.html
conf_ssl_cert_file_pem := SSL_CERT_FILE_PEM=$(current_dir)/certificate.pem
conf_ssl_key_file_pem := SSL_KEY_FILE_PEM=$(current_dir)/key.pem
conf_max_threads := MAX_THREADS=1024
conf_caching := CACHING_ENABLED=1
conf_interp := INTERPRETERS={.py:/usr/bin/python},{.php:/usr/bin/php}


all: http_server.o socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o str-utils/str-utils.o caching/caching.o hashtable/hashtable.o
	$(CC) http_server.o socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o \
	str-utils/str-utils.o caching/caching.o hashtable/hashtable.o \
	-lpthread -lm -lssl -lcrypto -pthread \
	$(CFLAGS) -o start-server
	
config:
	@echo GENERATING CONFIGS..
	@echo DO NOT COMMENT ON THE SAME LINE AS PARAMETERS > http.conf
	@echo $(conf_demo_dir) >> http.conf
	@echo $(conf_port_http) >> http.conf
	@echo $(conf_port_ssl) >> http.conf
	@echo $(conf_home) >> http.conf
	@echo $(conf_ssl_cert_file_pem) >> http.conf
	@echo $(conf_ssl_key_file_pem) >> http.conf
	@echo $(conf_max_threads) >> http.conf
	@echo $(conf_caching) >> http.conf
	@echo $(conf_interp) >> http.conf



http_server.o: http_server.c socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o str-utils/casing.h str-utils/str-utils.o caching/caching.o hashtable/hashtable.o sysout.h global-config.h
	$(CC) -c http_server.c -o http_server.o $(CFLAGS)

http-request.o: http-request.c http-request.h
	$(CC) -c http-request.c $(CFLAGS)

fileops.o: fileops/fileops.c fileops/fileops.h
	$(CC) -c fileops/fileops.c $(CFLAGS)

mime/http-mimes.o: mime/http-mimes.c mime/http-mimes.h
	$(CC) -c mime/http-mimes.c -o mime/http-mimes.o $(CFLAGS)

serversocket.o: socket/serversocket.c socket/serversocket.h
	$(CC) -c socket/serversocket.c $(CFLAGS)

http-ssl.o: socket/http-ssl.c socket/http-ssl.h
	$(CC) -c socket/http-ssl.c $(CFLAGS)

str-utils.o: str-utils/str-utils.c str-utils/str-utils.h
	$(CC) -c str-utils/str-utils.c $(CFLAGS)

caching.o: caching/caching.c caching/caching.h caching/hashtable.o
	$(CC) -c caching/caching.c $(CFLAGS)

hashtable.o: hashtable/hashtable.c hashtable/hashtable.h
	$(CC) -c caching/hashtable.c $(CFLAGS)

ssl-ca:
	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out certificate.pem
	openssl x509 -text -noout -in certificate.pem
	#openssl x509 -outform der -in certificate.pem -out HTTP-CA.crt -extensions v3_req


clean:
	rm socket/serversocket.o socket/http-ssl.o http-request.o fileops.o mime/http-mimes.o str-utils/str-utils.o http_server.o caching/*.o hashtable/*.o
