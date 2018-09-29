#ifndef _serversocket_h
#define _serversocket_h
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> //socket stuff
#include <sys/socket.h> //socket stuff
#include <netinet/in.h> //INADDR_ANY

struct server_socket {
    int fd;
    socklen_t len;
    struct sockaddr_in *handle;
};

struct server_socket create_server_socket(int port);
int send_data(int client_fd, char *data, size_t len);
void sock_cleanup(int client_fd);

#endif