#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //read()/write(), usleep()
#include <pthread.h> //multi-connection
#include <fcntl.h>  //to read html files
#include <string.h>
#include <sys/wait.h>
#include "serversocket.h"
#include "http-request.h"
#include "fileops.h"
#include "http-mimes.h"
#include "sysout.h"
#include "casing.h" //uppercase() and lowercase()
#include "strinsert.h" //strinsert()

#define MAX_REQUEST_LEN 20*1000*1024

//gcc http_server.c serversocket.c http-request.c fileops.c http-mimes.c -lpthread

/*Creating socket server in C:
 *socket() -> setsocketopt() -> bind() -> listen() -> accept()
 *then read/write into socket returned by accept()
 */

char SITEPATH[512] = ""; //physical path of website on disk
char HOMEPAGE[512] = ""; //default index page
int PORT = 80; //default port 80


void *conn_handler(void *fd);
void serve_static_content(int client_fd, char *local_uri, long content_len);
void send_error(int client_fd, int errcode);
int is_valid_method(char *method);
int load_global_config();
int generate_header(char *header, char *body, char *mime_type, char *content_len);

int main()
{
    int new_fd = 0; //session fd between client and server
    struct server_socket sock = create_server_socket(PORT);
    pthread_t pthread;
    int config_errno = load_global_config();
    switch (config_errno) {
        case -1:
            printf("Fatal error in http.conf: No PATH parameter specified.\n");
            return -1;
        case -2:
            printf("Fatal error in http.conf: Invalid SITEPATH parameter.\n");
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
//TODO: date, content-length, POST
void *conn_handler(void *fd)
{
    int client_fd = *(int*)fd;
    int n = 0;
    int bytes_read = 0;
    char buf[4096] = "";
    char local_uri[2048] = "";
    char mime_type[128] = "";
    char header[1024] = "";
    char content_len[16];
    long sz;
    char *request = NULL;
    char *_new_request;
    //printf("Connection established, fd: %d\n", client_fd);

    //PROCESS HTTP REQUEST FROM CLIENT
    /*read data via TCP stream
     *keep reading and allocating memory for new data until NULL or 20MB max reached
     *if buf[sizeof(buf)] != 0, there's still more data*/
    request = malloc(1); //request points to new data, must be freed during cleanup
    do {
        n = read_data(client_fd, buf, sizeof(buf));
        if (n == -1) {
            goto cleanup;
        }
        bytes_read += n;
        _new_request = (char*)realloc(request, bytes_read);
        if (_new_request == NULL) goto cleanup; //out of memory
        request = _new_request;
        memcpy(request + bytes_read - n, buf, n); //append data from tcp stream to *request
    } while (buf[sizeof(buf)] != 0 && bytes_read < MAX_REQUEST_LEN); //stop if request is larger than 20MB or reached NULL

    struct http_request req = process_request(request); //process returned data

    //SERVE CLIENT REQUEST
    //PROCESS URI (decode url, check privileges or if file exists)
    char decoded_uri[2048];
    if (strcmp(req.URI, "/") == 0) strcpy(req.URI, HOMEPAGE); //when URI is "/", e.g. GET / HTTP/1.1
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

    //TODO: cookie, gzip content, handle other methods like PUT, HEAD, DELETE..

    get_mime_type(mime_type, req.URI);
    
    //PROCESS DATA
    //handle executable requests here
    char interpreter[1024]; //path of interpreter program
    int is_interpretable = file_get_interpreter(local_uri, interpreter, sizeof(interpreter));
    if (is_interpretable > 0) { //CASE 1: uri is an executable file
        /*call interpreter, pass request body as argument*/
        char *p = local_uri;
        char *args[] = {interpreter, p, req.body, req.cookie, NULL};
        if (req.body_len) req.body[req.body_len] = 0; //NULL terminate upon GET with params, if it's POST, no body_len is returned
        int ret_code;
        char *data = (char*)system_output(args, &sz, &ret_code, 100000);
        if (ret_code < 0) { //if there's error in backend script, send err500 and skip sending returned data
            send_error(client_fd, 500);
            goto cleanup_data;
        }

        sprintf(content_len, "%d\n", sz); //equivalent to itoa(content_len)

        //generate header based on data returned from interpreter program
        generate_header(header, data, mime_type, content_len);

        //determine where the body is in returned data and send it to client
        char *doc;
        while (1) {
            doc = strstr(data, "\r\n\r\n");
            if (doc != NULL) break;
            doc = strstr(data, "\n\n");
            break;
        }
        if (doc) {
            //begin sending data via TCP
            send_data(client_fd, header, strlen(header)); //send header
            send_data(client_fd, doc, sz - (doc - data)); //send document
        }
        cleanup_data:
        free(data);
    }
    else if(is_interpretable == 0) { //CASE 2: uri is a static page
        sz = file_get_size(local_uri);
        strcpy(header, "HTTP/1.1 200 OK\n");
        strcat(header, "Content-Type: ");
        strcat(header, mime_type);
        
        strcat(header, "\nContent-Length: ");
        sprintf(content_len, "%d", sz);
        strcat(header, content_len);

        strcat(header, "\r\n\r\n"); //mandatory blank line separating header
        send_data(client_fd, header, strlen(header)); //send header
        serve_static_content(client_fd, local_uri, sz);
    }
    else { //error reading requested data or system can't interpret requested script
        send_error(client_fd, 500);
    }

    cleanup:
    //shutdown connection and free resources
    free(request);
    sock_cleanup(client_fd);
    return NULL;
}


//body includes everything that backend script prints out, not just html content
//*header is the processed output
int generate_header(char *header, char *body, char *mime_type, char *content_len)
{
    char buf[1024] = "";
    char user_defined_header[1024] = "";
    char httpver_final[32];
    char *doc_begin;
    char *n; //this will store linebreak char
    doc_begin = strstr(body, "\n<html");
    memcpy(buf, body, doc_begin - body);
    lowercase(user_defined_header, buf, doc_begin - body);

    //printf("%d %s\n--\n", doc_begin - body, user_defined_header);
    //fflush(stdout);

    strcpy(header, user_defined_header);

    //define content-type if back-end does not
    char *cont_type = strstr(user_defined_header, "content-type: ");
    if (!cont_type) {
        str_insert(header, 0, "content-type: ");
        str_insert(header, 14,mime_type); //based on known MIME types
        str_insert(header, strlen("content-type: ") + strlen(mime_type) ," ;charset=utf-8\n");
    }

    //define HTTP version if back-end doesn't specify
    char *httpver = strstr(user_defined_header, "http/");
    if (httpver) {
        n = strstr(httpver, "\n");
        if (n) {
            uppercase(httpver_final, httpver, n - httpver + 1); //n-httpver: len of httpver line
            memcpy(header, httpver_final, n - httpver + 1); //copy \n as well
        }
    }
    else { 
        str_insert(header, 0, "HTTP/1.1 200 OK\n"); //default HTTP code
    }


    //add content-length to header
    if (header[strlen(header) - 1] != '\n') { //if backend doesnt conclude their header with \n\n, do it for them
        strcat(header, "\r\n");
    }
    strcat(header, "Content-Length: ");
    strcat(header, content_len);
    strcat(header, "server: mini-http-server\r\n\r\n");

    return 0;
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


//read local_uri and write to client socket pointed to by client_fd
void serve_static_content(int client_fd, char *local_uri, long content_len)
{
    int bytes_read = 0;
    char response[4096];
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
}


void send_error(int client_fd, int errcode)
{
    char err404[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html; charset=UTF-8\r\n\r\n404\nResource not found on server.\n";
    char err403[] = "HTTP/1.1 403 FORBIDDEN\nContent-Type: text/html; charset=UTF-8\r\n\r\n403\nNot enough privilege to access resource.\n";
    char err400[] = "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/html; charset=UTF-8\r\n\r\n400\nBad request.\n";
    char err500[] = "HTTP/1.1 500 INTERNAL SERVER ERROR\nContent-Type: text/html; charset=UTF-8\r\n\r\n500\nInternal Error.\n";

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
    memcpy(SITEPATH, s, lnbreak - s);
    if (!is_dir(SITEPATH)) return -2; //invalid SITEPATH

    //PORT: port to listen on (default 80)
    s = strstr(buf, "PORT=");
    if (s == NULL) PORT = 80; //no PORT config, use default 80
    s += 5; //len of "PORT="
    PORT = atoi(s);

    //HOMEPAGE: default index page
    s = strstr(buf, "HOME=");
    if (s == NULL) {
        strcpy(HOMEPAGE, "/index.html");
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 5;
        memcpy(HOMEPAGE, s, lnbreak - s);
    }
    
    //other configs below

    return 0;
}
