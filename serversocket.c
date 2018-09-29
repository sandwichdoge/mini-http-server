#include "serversocket.h"

//create a TCP server socket, return info about that socket
struct server_socket create_server_socket(int port)
{
    int err = 0;
    struct server_socket ret;
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    socklen_t len = sizeof(server);

    int fd = socket(AF_INET, SOCK_STREAM, 0); //Inet TCP
    if (!fd) printf("error creating socket\n");
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); //optional, prevents address already in use error
    err = bind(fd, (struct sockaddr*)&server, sizeof(server));
    if (err < 0) printf("error binding\n");
    err = listen(fd, 128); //max connections
    if (err < 0) printf("error listening\n");

    ret.fd = fd;
    ret.len = len;
    ret.handle = &server;
    return ret;
}


//more robust way to send data via tcp
int send_data(int client_fd, char *data, size_t len)
{
    int bytes_written = 0;
    while (bytes_written < len) { //in case of traffic congestion
        bytes_written += write(client_fd, data, len); //send tcp response
        if (bytes_written == -1) break; //error sending data to client
    }
    return bytes_written;
}


 //close connection and clean up resources
void sock_cleanup(int client_fd)
{
    //printf("Disconnecting client: %d\n", client_fd);
    //fflush(stdout);
    shutdown(client_fd, 2); //shutdown connection
    close(client_fd);
}
