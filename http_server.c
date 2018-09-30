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
#define MAX_REQUEST_LEN 20*1000*1024

//gcc http_server.c serversocket.c http-request.c fileops.c http-mimes.c -lpthread

/*Creating socket server in C:
 *socket() -> setsocketopt() -> bind() -> listen() -> accept()
 *then read/write into socket returned by accept()
 */

char SITEPATH[512] = ""; //physical path of website on disk
int PORT = 80; //default port 80


void *conn_handler(void *fd);
void send_error(int client_fd, int errcode);
int is_valid_method(char *method);
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

//something wrong with std realloc()
void *_realloc(void *ptr, size_t newsz)
{
    void *ret = malloc(newsz);
    if (!ret) return NULL;
    memcpy(ret, ptr, newsz);
    free(ptr);
    return ret;
}


//handle an incoming connection
//return nothing
//*fd is pointer the file descriptor of client socket
//TODO: date, content-length, POST
void *conn_handler(void *fd)
{
    int client_fd = *(int*)fd;
    int bytes_read = 0;
    int n = 0;
    char buf[4096] = "";
    char response[4096];
    char local_uri[1024] = "";
    char mime_type[128] = "";
    char header[1024] = "";
    char *request = NULL;
    char *_new_request;

    //printf("Client connected, fd: %d\n", client_fd);

    //PROCESS HTTP REQUEST
    /*if buf[sizeof(buf)] != 0, there's still more data*/
    request = malloc(1);
    do {
        n = read_data(client_fd, buf, sizeof(buf));
        if (n == -1) {
            goto cleanup;
        }
        bytes_read += n;
        _new_request = (char*)_realloc(request, bytes_read);
        if (_new_request) request = _new_request;
        else goto cleanup; //out of memory
        memcpy(request + bytes_read - n, buf, n); //append data from tcp stream to request
    } while (buf[sizeof(buf)] != 0 && bytes_read < MAX_REQUEST_LEN); //if request is larger than 20MB then stop
    
    struct http_request req = process_request(request);
    //printf("method:%s\nuri:%s\nhttpver:%s\n", req.method, req.URI, req.httpver);
    //fflush(stdout);

    //SERVE CLIENT REQUEST    
    //PROCESS URI (decode url, check privileges or if file exists)
    char decoded_uri[1024];
    if (strcmp(req.URI, "/") == 0) strcpy(req.URI, "/index.html");
    decode_url(decoded_uri, req.URI); //decode url (%69ndex.html -> index.html)

    strcpy(local_uri, SITEPATH); //allow only resources in site directory
    strncat(local_uri, decoded_uri, sizeof(decoded_uri) - sizeof(SITEPATH)); //local_uri: local path of requested resource

    if (!is_valid_method(req.method)) {
        send_error(client_fd, 400); //then send 400 to client
        goto cleanup;
    }

    if (file_exists(local_uri) < 0) { //if local resource doesn't exist
        send_error(client_fd, 404); //then send 404 to client
        goto cleanup;
    }
    if (file_readable(local_uri) < 0) { //local resource isn't read-accessible
        send_error(client_fd, 403);  //then send 403 to client
        goto cleanup;
    }

    //TODO: handle other methods like POST, PUT, HEAD..

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
    bytes_read = 0;

    while (bytes_written < content_len) {
        memset(response, 0, sizeof(response));
        bytes_read = read(content_fd, response, sizeof(response)); //content body
        bytes_written += send_data(client_fd, response, bytes_read);
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

    close(content_fd);
    cleanup:
    //shutdown connection and free resources
    free(request);
    sock_cleanup(client_fd);
    return NULL;
}


//return 0 if request method is not valid
int is_valid_method(char *method)
{
    char *methods[] = {"GET", "POST", "HEAD", "PUT", "DELETE", "PATCH", "CONNECT", "OPTIONS"};
    int total = sizeof(methods) / sizeof(methods[0]);
    for (int i = 0; i < total; ++i) {
        if (strcmp(method, methods[i]) == 0) return 1;
    }
    return 0;
}


void send_error(int client_fd, int errcode)
{
    char err404[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html; charset=UTF-8\r\n\r\n404\nResource not found on server.\n";
    char err403[] = "HTTP/1.1 403 FORBIDDEN\nContent-Type: text/html; charset=UTF-8\r\n\r\n403\nNot enough privilege to access resource.\n";
    char err400[] = "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/html; charset=UTF-8\r\n\r\n400\nBad request.\n";

    switch (errcode) {
        case 404:
            send_data(client_fd, err404, sizeof(err404));
            break;
        case 403:
            send_data(client_fd, err403, sizeof(err403));
            break;
        case 400:
            send_data(client_fd, err400, sizeof(err400));
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
