#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //read()/write(), usleep()
#include <sys/types.h> //socket stuff
#include <sys/socket.h> //socket stuff
#include <netinet/in.h> //INADDR_ANY
#include <pthread.h> //multi-connection
#include <fcntl.h>  //to read html files
#include <string.h>

/*Creating socket server in C:
 *socket() -> setsocketopt() -> bind() -> listen() -> accept()
 *then read/write into socket returned by accept()
 */

char g_sitepath[128];

struct server_socket {
    int fd;
    socklen_t len;
    struct sockaddr_in *handle;
};

struct http_request {
    char method[8];
    char URI[1024];
    char httpver[16];
};

void *conn_handler(void *fd);
struct server_socket create_server_socket(int port);
struct http_request process_request(char *request);
long file_get_size(char *path);
int file_exists(char *path);
int send_data(int client_fd, char *data, size_t len);
void send_error(int client_fd, int errcode);
void decode_url(char *out, char *url);
void cleanup(int client_fd);
void load_config();


int main()
{
    int port = 80;
    int new_fd = 0;
    struct server_socket sock = create_server_socket(port);
    pthread_t pthread;
    load_config();
    printf("Started HTTP server on port %d\n", port);
    while (1) {
        new_fd = accept(sock.fd, (struct sockaddr*)sock.handle, &sock.len);
        if (new_fd > 0) {
            pthread_create(&pthread, NULL, conn_handler, &new_fd); //create a thread for each new connection
            usleep(1000); //handle racing condition
        }
    }
    printf("Server stopped\n");
    
    return 0;
}


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
    err = listen(fd, 16); //max connections
    if (err < 0) printf("error listening\n");

    ret.fd = fd;
    ret.len = len;
    ret.handle = &server;
    return ret;
}


//handle an incoming connection
//return nothing
//*fd is pointer the file descriptor of client socket
//TODO: MIME types, date, content-length
void *conn_handler(void *fd)
{
    int client_fd = *(int*)fd;
    static char request[4096] = "";
    static char response[4096] = "";
    static char local_uri[1024] = "";
    static char header[1024] = "HTTP/1.1 200 OK\nContent-Type: text/html; charset=UTF-8\n\n"
    "<!DOCTYPE html PUBLIC>";
    
    printf("Client connected, fd: %d\n", client_fd);

    //PROCESS HTTP HEADER
    if (read(client_fd, request, sizeof(request)) == -1) cleanup(client_fd);
    struct http_request req = process_request(request);
    printf("method:%s\nuri:%s\nhttpver:%s\n", req.method, req.URI, req.httpver);
    fflush(stdout);


    //SERVE CLIENT REQUEST    
    //PROCESS URI (decode url, check privileges or if file exists)
    char decoded_uri[1024];
    if (strcmp(req.URI, "/") == 0) strcpy(req.URI, "/index.html");
    decode_url(decoded_uri, req.URI); //decode url (%69ndex.html -> index.html)

    strcpy(local_uri, g_sitepath); //allow only resources in site directory
    strncat(local_uri, decoded_uri, sizeof(decoded_uri) - sizeof(g_sitepath));
    if (file_exists(local_uri) < 0) { //if local resource doesn't exist
        send_error(client_fd, 404); //then send 404 to client
        cleanup(client_fd);
        return NULL; //stop;
    }

    //SEND HEADER
    send_data(client_fd, header, sizeof(header)); //send header

    //SEND BODY
    int bytes_written = 0;
    int html_fd = open(local_uri, O_RDONLY); //get requested file content
    long html_len = file_get_size(local_uri);
    printf("content-length: %d\n", html_len);

    while (bytes_written < html_len) {
        memset(response, 0, sizeof(response));
        read(html_fd, response, sizeof(response)); //html body
        bytes_written += send_data(client_fd, response, sizeof(response));
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

    close(html_fd);

    //shutdown connection and free resources
    cleanup(client_fd);
}


//return info about request like method, uri, http version, etc.
struct http_request process_request(char *request)
{
    struct http_request ret;
    int method_len = 0;
    int uri_len = 0;
    int httpver_len = 0;

    memset(ret.URI, 0, sizeof(ret.URI));
    memset(ret.method, 0, sizeof(ret.method));
    memset(ret.httpver, 0, sizeof(ret.httpver));

    for (method_len; request[method_len] != ' ' && method_len <= sizeof(ret.method); ++method_len);
    memcpy(ret.method, request, method_len);
    ret.method[method_len] = 0; //NULL terminate

    for (uri_len = 0; request[method_len + uri_len + 1] != ' ' && uri_len <= sizeof(ret.URI); ++uri_len); //next is the requested URI, after space
    memcpy(ret.URI, request+method_len+1, uri_len);
    ret.URI[uri_len] = 0; //NULL terminate

    for (httpver_len = 0; request[method_len + uri_len + httpver_len + 1] != '\n' && httpver_len <= sizeof(ret.httpver); ++httpver_len); //next is httpver if specified
    memcpy(ret.httpver, request+method_len+uri_len+2, httpver_len);
    ret.httpver[httpver_len] = 0; //NULL terminate

    //next line of http header
    //accept-encoding, user-agent, referer, accept-language
    char *header_fields = request+method_len+uri_len+httpver_len+2;
    //printf("%s\n", request);
    fflush(stdout);
    return ret;
}


//load global config like site directory
void load_config()
{
    char buf[4096];
    int fd = open("http.conf", O_RDONLY);
    read(fd, buf, sizeof(buf));
    char *sitepath = strstr(buf, "PATH=");
    sitepath += 5; //"PATH=" len
    strcpy(g_sitepath, sitepath);

}


int file_exists(char *path)
{
    return access(path, F_OK);
}


long file_get_size(char *path)
{
    FILE *fd = fopen(path, "r");
    fseek(fd, 0L, SEEK_END);
    return ftell(fd);
}


//more robust way to send data via tcp
int send_data(int client_fd, char *data, size_t len)
{
    int bytes_written = 0;
    while (bytes_written < len) { //handle traffic congestion
        bytes_written += write(client_fd, data, len); //send http response
        if (bytes_written == -1) break; //error sending data to client
    }
    return bytes_written;
}


void send_error(int client_fd, int errcode)
{
    char err404[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html; charset=UTF-8\n\n404\nResource not found on server.";
    switch (errcode) {
        case 404:
            send_data(client_fd, err404, sizeof(err404));
            break;
    }
}


 //close connection and clean up resources
void cleanup(int client_fd)
{
    printf("Disconnecting client: %d\n", client_fd);
    fflush(stdout);
    shutdown(client_fd, 2); //shutdown connection
    close(client_fd);
}


//%69ndex.html => index.html
void decode_url(char *out, char *url)
{
    char buf[3];
    char c;
    for (int i = 0; url[i]; ++i) {
        if (url[i] == '%') {
            i++;
            memcpy(buf, &url[i], 2);
            c = (char)strtoul(buf, 0, 16);
            i += 1;
        }
        else c = url[i];
        *(out++) = c;
    }
}