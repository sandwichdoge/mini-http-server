#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //read()/write(), usleep()
#include <pthread.h> //multi-connection
#include <fcntl.h>  //to read html files
#include <string.h>
#include "serversocket.h"
#include "http-request.h"
#include "fileops.h"
#include "http-mimes.h"

//gcc http_server.c serversocket.c http-request.c fileops.c http-mimes.c -lpthread

/*Creating socket server in C:
 *socket() -> setsocketopt() -> bind() -> listen() -> accept()
 *then read/write into socket returned by accept()
 */

char SITEPATH[512] = ""; //physical path of website on disk
int PORT = 80; //default port 80


void *conn_handler(void *fd);
void send_error(int client_fd, int errcode);
int load_global_config();


int main()
{
    int new_fd = 0;
    struct server_socket sock = create_server_socket(PORT);
    pthread_t pthread;
    if (load_global_config() < 0) {
        printf("Fatal error in http.conf\n");
        return -1;
    }

    printf("Started HTTP server on port %d..\n", PORT);
    while (1) {
        new_fd = accept(sock.fd, (struct sockaddr*)sock.handle, &sock.len);
        if (new_fd > 0) {
            pthread_create(&pthread, NULL, conn_handler, &new_fd); //create a thread for each new connection
            usleep(2000); //handle racing condition
        }
    }
    printf("Server stopped.\n");
    
    return 0;
}


//handle an incoming connection
//return nothing
//*fd is pointer the file descriptor of client socket
//TODO: MIME types, date, content-length
void *conn_handler(void *fd)
{
    int client_fd = *(int*)fd;
    char request[4096] = "";
    char response[4096];
    char local_uri[1024] = "";
    char mime_type[128] = "";
    char header[1024] = "";

    //printf("Client connected, fd: %d\n", client_fd);

    //PROCESS HTTP HEADER
    if (read(client_fd, request, sizeof(request)) == -1) sock_cleanup(client_fd);
    struct http_request req = process_request(request);
    //printf("method:%s\nuri:%s\nhttpver:%s\n", req.method, req.URI, req.httpver);
    //fflush(stdout);

    //SERVE CLIENT REQUEST    
    //PROCESS URI (decode url, check privileges or if file exists)
    char decoded_uri[1024];
    if (strcmp(req.URI, "/") == 0) strcpy(req.URI, "/index.html");
    decode_url(decoded_uri, req.URI); //decode url (%69ndex.html -> index.html)

    strcpy(local_uri, SITEPATH); //allow only resources in site directory
    strncat(local_uri, decoded_uri, sizeof(decoded_uri) - sizeof(SITEPATH));

    if (file_exists(local_uri) < 0) { //if local resource doesn't exist
        send_error(client_fd, 404); //then send 404 to client
        goto cleanup;
    }
    if (file_readable(local_uri) < 0) { //local resource isn't read-accessible
        send_error(client_fd, 403);  //then send 403 to client
        goto cleanup;
    }

    //SEND HEADER
    //TODO: better code to handle response header
    get_mime_type(mime_type, req.URI);
    strcpy(header, "HTTP/1.1 200 OK\n");
    strcat(header, "Content-Type: ");
    strcat(header, mime_type);
    strcat(header, "; charset=UTF-8");
    strcat(header, "\r\n\r\n");
    send_data(client_fd, header, strlen(header)); //send header


    //SEND BODY
    long content_len = file_get_size(local_uri);
    //printf("Content-length: %d\n", content_len);
    int content_fd = open(local_uri, O_RDONLY); //get requested file content
    int bytes_written = 0;
    int bytes_read = 0;

    while (bytes_written < content_len) {
        memset(response, 0, sizeof(response));
        bytes_read = read(content_fd, response, sizeof(response)); //content body
        bytes_written += send_data(client_fd, response, bytes_read);
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

    close(content_fd);

    cleanup:
    //shutdown connection and free resources
    sock_cleanup(client_fd);
}


void send_error(int client_fd, int errcode)
{
    char err404[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html; charset=UTF-8\r\n\r\n404\nResource not found on server.";
    char err403[] = "HTTP/1.1 403 FORBIDDEN\nContent-Type: text/html; charset=UTF-8\r\n\r\n403\nNot enough privilege to access resource.";
    switch (errcode) {
        case 404:
            send_data(client_fd, err404, sizeof(err404));
            break;
        case 403:
            send_data(client_fd, err403, sizeof(err403));
            break;
    }
}


/*load global configs
 *SITEPATH: physical path of website on disk
 *PORT: port to listen on (default 80)
 */
int load_global_config()
{
    char buf[4096];
    char *s;
    char *lnbreak;

    int fd = open("http.conf", O_RDONLY);
    read(fd, buf, sizeof(buf)); //read http.conf file into buf

    //SITEPATH: physical path of website
    s = strstr(buf, "PATH=");
    lnbreak = strstr(s, "\n");
    if (s == NULL) return -1; //no PATH config
    s += 5; //len of "PATH="
    memset(SITEPATH, 0, sizeof(SITEPATH));
    memcpy(SITEPATH, s, lnbreak - s);
    if (!is_dir(SITEPATH)) return -2; //no PATH specified
    printf("%s\n", SITEPATH);

    //PORT: port to listen on (default 80)
    s = strstr(buf, "PORT=");
    if (s == NULL) return 0; //no PORT config, use default 80
    s += 5; //len of "PORT="
    PORT = atoi(s);

    //other configs below

    return 0;
}