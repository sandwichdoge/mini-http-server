#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //read()/write(), usleep()
#include <pthread.h> //multi-connection
#include <fcntl.h>  //to read html files
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include "socket/serversocket.h"
#include "socket/http-ssl.h"
#include "http-request.h"
#include "fileops.h"
#include "mime/http-mimes.h"
#include "sysout.h"
#include "casing.h" //uppercase() and lowercase()
#include "str-utils.h" //strinsert()

#define MAX_REQUEST_LEN 20*1000*1024

//gcc http_server.c serversocket.c http-request.c fileops.c http-mimes.c -lpthread

/*Creating socket server in C:
 *socket() -> setsocketopt() -> bind() -> listen() -> accept()
 *then read/write into socket returned by accept()
 */
/*Implementing OpenSSL server in C:
 *Initialize SSL - SSL_library_init(), load_error_strings(), OpenSSL_add_all_algorithms()
 *Create CTX: SSL_CTX_new(method()) -> SSL_CTX_set_options()
 *SSL_CTX_use_certificate_file(), SSL_CTX_use_PrivateKey_file() -> SSL_CTX_check_private_key() to validate
 *---And we're done with global initialization---
 *---Connection ahead, require a client fd returned from accept() syscall---
 *ssl_connection = SSL_new(), ssl_set_fd() to link client_fd with ssl_connection
 *ssl_accept() to wait for client to initiate handshake, non-block I/O client_fd to implement timeout otherwise it'll hang on bad requests
 */

void *conn_handler(void *fd);
void serve_static_content(int client_fd, char *local_uri, long content_len, SSL *conn_SSL);
void http_send_error(int client_fd, int errcode, SSL *conn_SSL);
int is_valid_method(char *method);
int load_global_config();
int generate_header(char *header, char *body, char *mime_type, char *content_len);


typedef struct client_info {
    int is_ssl;
    int client_fd;
} client_info;

char SITEPATH[512] = ""; //physical path of website on disk
char HOMEPAGE[512] = ""; //default index page
char CERT_PUB_KEY_FILE[512];
char CERT_PRIV_KEY_FILE[512];
int PORT = 80; //default port 80
int PORT_SSL = 443; //default port for SSL is 443
SSL_CTX *CTX;

void handle_SIGABRT()
{
    fprintf(stderr, "SIGABRT received\n"); fflush(stderr);
}

int main()
{
    int config_errno = load_global_config();
    switch (config_errno) {
        case -1:
            printf("Fatal error in http.conf: No PATH parameter specified.\n");
            return -1;
        case -2:
            printf("Fatal error in http.conf: Invalid SITEPATH parameter.\n");
            return -1;
        case -3:
            printf("Fatal error: cannot open http.conf\n");
    }

    struct server_socket sock = create_server_socket(PORT);
    struct server_socket sock_ssl = create_server_socket(PORT_SSL);
    int http_fd= 0, ssl_fd = 0; //session fd between client and server
    client_info args;
    pthread_t pthread;


    initialize_SSL();
    CTX = SSL_CTX_new(TLS_server_method());
    if (!CTX) {
        printf("Error creating CTX.\n");
    }
    SSL_CTX_set_options(CTX, SSL_OP_SINGLE_DH_USE);
    //SSL_CTX_set_session_cache_mode(CTX, SSL_SESS_CACHE_SERVER); CACHE_SERVER is default setting, no need to set again


    SSL_CTX_use_certificate_file(CTX, CERT_PUB_KEY_FILE , SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(CTX, CERT_PRIV_KEY_FILE, SSL_FILETYPE_PEM);
    if (!SSL_CTX_check_private_key(CTX)) {
        fprintf(stderr,"SSL WARNING: Private key does not match certificate public key.\n");
    }
    
    printf("Started HTTP server on port %d and %d..\n", PORT, PORT_SSL);
    pid_t pid = fork(); //1 thread for HTTP, 1 for HTTPS
    if (pid == 0) {
        while (1) {
            ssl_fd = accept(sock_ssl.fd, (struct sockaddr*)sock_ssl.handle, &sock_ssl.len);
            if (ssl_fd> 0) {
                args.is_ssl = 1;
                args.client_fd = ssl_fd;
                /*Handle SIGPIPE signals, caused by writing to connection that's already closed by client*/
                signal(SIGPIPE, SIG_IGN);
                pthread_create(&pthread, NULL, conn_handler, (void*)&args); //create a thread for each new connection
            }
            usleep(5000); //handle racing condition. TODO: better than this
        }
    }
    else {
        while (1) {
            http_fd = accept(sock.fd, (struct sockaddr*)sock.handle, &sock.len);
            if (http_fd> 0) {
                args.is_ssl = 0;
                args.client_fd = http_fd;
                /*Handle SIGPIPE signals, caused by writing to connection that's already closed by client*/
                signal(SIGPIPE, SIG_IGN);
                pthread_create(&pthread, NULL, conn_handler, (void*)&args); //create a thread for each new connection
            }
            usleep(5000);
        }
    }

    printf("Server stopped.\n");
    shutdown_SSL();
    return 0;
}


//handle an incoming connection
//return nothing
//*fd is pointer the file descriptor of client socket
//TODO: date, proper log
void *conn_handler(void *vargs)
{
    client_info *args = (client_info*) vargs;
    int client_fd = args->client_fd;
    int is_ssl = args->is_ssl;
    int bytes_read = 0;
    int ssl_err = 0;
    char buf[4096*2] = "";
    char local_uri[2048] = "";
    char mime_type[128] = "";
    char header[1024] = "";
    char content_len[16];
    long sz;
    char *request = NULL;
    char *_new_request;
    SSL *conn_SSL = NULL;
    //printf("Connection established, fd: %d\n", client_fd); fflush(stdout);

    /*Initialize SSL connection*/
    if (is_ssl) {
        fcntl(client_fd, F_SETFL, O_NONBLOCK); //non-blocking IO for timeout measure
        conn_SSL = SSL_new(CTX);
        if (!conn_SSL) {
            fprintf(stderr, "Error creating SSL.\n"); fflush(stderr);
            ssl_err = 2;
            goto cleanup;
        }
        SSL_set_fd(conn_SSL, client_fd);
        SSL_set_accept_state(conn_SSL);

        int counter = 200; //timeout counter: 10s
        do {
            ssl_err = SSL_get_error(conn_SSL, SSL_accept(conn_SSL));
            usleep(50000); //check state every 0.05s
            --counter;
        } while (ssl_err == 2 && counter > 0); //ssl_err == 2 means handshake is still in process

        if (ssl_err) {
            fprintf(stderr, "Error accepting SSL handshake err:%d, fd:%d\n", ssl_err, client_fd); fflush(stderr);
            goto cleanup;
        }
    }

    //PROCESS HTTP REQUEST FROM CLIENT
    /*read data via TCP stream, append new data to heap
     *keep reading and allocating memory for new data until NULL or 20MB max reached
     *finally pass it it down to process_request() to get info about it (uri, method, body data)
     *if buf[sizeof(buf)] != 0, there's still more data*/
    int n = 0;
    request = malloc(1); //request points to new data, must be freed during cleanup
    do {
        if (is_ssl) { //https
            n = read_data_ssl(conn_SSL, buf, sizeof(buf));
        }
        else { //regular http without ssl
            n = read_data(client_fd, buf, sizeof(buf));
        }
        if (n < 0) goto cleanup;
        
        if (n > 0) {
            bytes_read += n;
            _new_request = (char*)realloc(request, bytes_read + 1);
            if (_new_request == NULL) {
                fprintf(stderr, "Out of memory.\n"); fflush(stderr);
                goto cleanup; //out of memory
            }
            request = _new_request;
            memcpy(request + bytes_read - n, buf, n); //append data from tcp stream to *request
        }
        
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
        http_send_error(client_fd, 400, conn_SSL); //then send 400 to client
        goto cleanup;
    }

    if (file_exists(local_uri) < 0) { //if local resource doesn't exist
        http_send_error(client_fd, 404, conn_SSL); //then send 404 to client
        goto cleanup;
    }
    if (file_readable(local_uri) < 0) { //local resource isn't read-accessible
        http_send_error(client_fd, 403, conn_SSL);  //then send 403 to client
        goto cleanup;
    }

    //printf("res:%s\n", local_uri); fflush(stdout);
    //TODO: cookie, gzip content, handle other methods like PUT, HEAD, DELETE..
    
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
        char *data = system_output(args, &sz, &ret_code, 20000); //20s timeout on backend script
        if (ret_code < 0) { //if there's error in backend script, send err500 and skip sending returned data
            http_send_error(client_fd, 500, conn_SSL);
            goto cleanup_data;
        }
        
        get_mime_type(mime_type, req.URI); //MIME type for response header
        sprintf(content_len, "%d\n", sz); //content-length for response header - equivalent to itoa(content_len)

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
            if (is_ssl) {
                send_data_ssl(conn_SSL, header, strlen(header)); //send header
                send_data_ssl(conn_SSL, doc, sz - (doc - data)); //send document
            }
            else {
                send_data(client_fd, header, strlen(header)); //send header
                send_data(client_fd, doc, sz - (doc - data)); //send document
            }
            
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
        if (is_ssl) {
            send_data_ssl(conn_SSL, header, strlen(header));
        }
        else {
            send_data(client_fd, header, strlen(header)); //send header
        }
        serve_static_content(client_fd, local_uri, sz, conn_SSL);
    }
    else { //error reading requested data or system can't interpret requested script
        http_send_error(client_fd, 500, conn_SSL);
    }
    
    cleanup:
    //shutdown connection and free resources
    if (!ssl_err) free(request); //free() shouldn't do anything if pointer is NULL, but in multithreading it's funky, so check jic
    if (is_ssl && conn_SSL != NULL) disconnect_SSL(conn_SSL, ssl_err); //will call additional SSL_free() if there's error (ssl_err != 0)
    sock_cleanup(client_fd);
    //printf("Cleaned up.\n"); fflush(stdout);

    return NULL;
}


//body includes everything that backend script prints out, not just html content
//*header is the processed output
//*body is IMMUTABLE
int generate_header(char *header, char *body, char *mime_type, char *content_len)
{
    char buf[1024] = "";
    char user_defined_header[1024] = "";
    char httpver_final[32];
    char *doc_begin;
    char *n; //this will store linebreak char

    doc_begin = strstr(body, "\n<html");
    if (doc_begin == NULL) return -1; //invalid html response, callee will throw 500
    strncpy(buf, body, doc_begin - body);
    lowercase(user_defined_header, buf, doc_begin - body);

    //printf("%d %s\n--\n", doc_begin - body, user_defined_header);
    //fflush(stdout);

    strncpy(header, user_defined_header, sizeof(user_defined_header));
    
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


//return 0 if request method is not valid (any request that doesn't belong the methods set)
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
void serve_static_content(int client_fd, char *local_uri, long content_len, SSL *conn_SSL)
{
    int bytes_read = 0;
    char response[4096];
    int content_fd = open(local_uri, O_RDONLY); //get requested file content
    int bytes_written = 0;
    bytes_read = 0;

    while (bytes_written < content_len) {
        memset(response, 0, sizeof(response));
        bytes_read = read(content_fd, response, sizeof(response)); //content body
        if (conn_SSL) {
            bytes_written += send_data_ssl(conn_SSL, response, bytes_read);
        }
        else {
            bytes_written += send_data(client_fd, response, bytes_read);
        }
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

    close(content_fd);
}


void http_send_error(int client_fd, int errcode, SSL *conn_SSL)
{
    char err404[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html; charset=UTF-8\r\n\r\n404\nResource not found on server.\n";
    char err403[] = "HTTP/1.1 403 FORBIDDEN\nContent-Type: text/html; charset=UTF-8\r\n\r\n403\nNot enough privilege to access resource.\n";
    char err400[] = "HTTP/1.1 400 BAD REQUEST\nContent-Type: text/html; charset=UTF-8\r\n\r\n400\nBad request.\n";
    char err500[] = "HTTP/1.1 500 INTERNAL SERVER ERROR\nContent-Type: text/html; charset=UTF-8\r\n\r\n500\nInternal Error.\n";

    switch (errcode) {
        case 500:
            if (conn_SSL) send_data_ssl(conn_SSL, err500, sizeof(err500));
            else send_data(client_fd, err500, sizeof(err500));
            break;      
        case 404:
            if (conn_SSL) send_data_ssl(conn_SSL, err404, sizeof(err404));
            else send_data(client_fd, err404, sizeof(err404));
            break;
        case 403:
            if (conn_SSL) send_data_ssl(conn_SSL, err403, sizeof(err403));
            else send_data(client_fd, err403, sizeof(err403));
            break;
        case 400:
            if (conn_SSL) send_data_ssl(conn_SSL, err400, sizeof(err400));
            else send_data(client_fd, err400, sizeof(err400));
            break;       
    }
}


/*load global configs
 *SITEPATH: physical path of website on disk
 *PORT: port to listen on (default 80)
 *HOME: default homepage
 *SSL_CERT_FILE_PEM: path of PEM file used for SSL that contains public key
 *SSL_KEY_FILE_PEM: path of PEM file used for SSL that contains private key
 */
int load_global_config()
{
    char buf[4096];
    char *s;
    char *lnbreak;

    int fd = open("http.conf", O_RDONLY);
    if (fd < 0) return -3; //could not open http.conf
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

    //PORT for SSL (default 443)
    s = strstr(buf, "PORT_SSL=");
    if (s == NULL) PORT_SSL = 443; //no PORT config, use default 80
    s += 9; //len of "PORT_SSL="
    PORT_SSL = atoi(s);

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
    
    //SSL stuff
    s = strstr(buf, "SSL_CERT_FILE_PEM="); //public key file path
    if (s == NULL) {
        CERT_PUB_KEY_FILE[0] = 0;
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 18; // len of "SSL_CERT_FILE_PEM="
        memcpy(CERT_PUB_KEY_FILE, s, lnbreak - s);
    }

    s = strstr(buf, "SSL_KEY_FILE_PEM="); //private key file path
    if (s == NULL) {
        CERT_PRIV_KEY_FILE[0] = 0;
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 17; // len of "SSL_CERT_FILE_PEM="
        memcpy(CERT_PRIV_KEY_FILE, s, lnbreak - s);
    }

    //other configs below

    return 0;
}
