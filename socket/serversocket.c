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
    if (!fd) fprintf(stderr, "Error creating socket\n");
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); //optional, prevents address already in use error
    
    err = bind(fd, (struct sockaddr*)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Error binding. Requires SU privileges if port < 1024. Or is port busy?\n");
        fd = -1;
    }

    err = listen(fd, MAX_CONNECTIONS); //max connections
    if (err < 0) {
        fprintf(stderr, "Error listening.\n");
        fd = -1;
    }
    
    ret.fd = fd;
    ret.len = len;
    ret.handle = &server;
    return ret;
}


/*read data via tcp*/
int read_data(int client_fd, char *buf, size_t bufsize)
{
    memset(buf, 0, bufsize);
    return read(client_fd, buf, bufsize);
}


//more robust way to send data via tcp
int send_data(int client_fd, char *data, size_t len)
{
    int bytes_written = 0;
    do {
        bytes_written += write(client_fd, data, len - bytes_written); //send tcp response
        if (bytes_written < 0) break; //error sending data to client
    } while (bytes_written < len); //handle partial write in case of traffic congestion

    return bytes_written;
}


int read_data_ssl(SSL *SSL_conn, char *buf, size_t bufsize)
{
    memset(buf, 0, bufsize);
    return SSL_read(SSL_conn, buf, bufsize);
}


int send_data_ssl(SSL *SSL_conn, char *data, size_t len)
{
    int bytes_written;
    if (SSL_get_shutdown(SSL_conn) > 0) bytes_written = -1;
    else bytes_written = SSL_write(SSL_conn, data, len); //all or nothing, no partial write in SSL
    //SSL_MODE_ENABLE_PARTIAL_WRITE for partial write
    return bytes_written;
}

 //close connection and clean up resources
void sock_cleanup(int client_fd)
{
    shutdown(client_fd, 2); //shutdown connection
    close(client_fd);
}
