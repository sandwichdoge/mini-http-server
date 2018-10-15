current_dir = $(shell pwd)
conf_demo_dir := PATH=$(current_dir)
conf_demo_dir := $(conf_demo_dir)/demo-site-1
conf_port_http := PORT=80
conf_port_ssl := PORT_SSL=443
conf_home := HOME=/index.html
conf_ssl_cert_file_pem := SSL_CERT_FILE_PEM=$(current_dir)/certificate.pem
conf_ssl_key_file_pem := SSL_KEY_FILE_PEM=$(current_dir)/key.pem


all: http_server.o serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o
	gcc -g http_server.o serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o -lpthread -lssl -lcrypto -o start-server
	@echo GENERATING CONFIGS..
	@echo DO NOT COMMENT ON THE SAME LINE AS PARAMETERS > http.conf
	@echo $(conf_demo_dir) >> http.conf
	@echo $(conf_port_http) >> http.conf
	@echo $(conf_port_ssl) >> http.conf
	@echo $(conf_home) >> http.conf
	@echo $(conf_ssl_cert_file_pem) >> http.conf
	@echo $(conf_ssl_key_file_pem) >> http.conf


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


ssl-ca:
	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out certificate.pem
	openssl x509 -text -noout -in certificate.pem


clean:
	rm serversocket.o http-request.o fileops.o http-mimes.o http-ssl.o http_server.o