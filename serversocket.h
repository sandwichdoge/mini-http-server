#ifndef _serversocket_h
#define _serversocket_h
#include <stdio.h>
#include <unistd.h>
#include <string.h> //memset()
#include <sys/types.h> //socket stuff
#include <sys/socket.h> //socket stuff
#include <netinet/in.h> //INADDR_ANY
#include "http-ssl.h"

struct server_socket {
    int fd;
    socklen_t len;
    struct sockaddr_in *handle;
};

struct server_socket create_server_socket(int port);
int send_data(int client_fd, char *data, size_t len);
int read_data(int client_fd, char *buf, size_t bufsize);
int send_data_ssl(SSL *SSL_conn, char *data, size_t len);
int read_data_ssl(SSL *SSL_conn, char *buf, size_t bufsize);
void sock_cleanup(int client_fd);

#endif