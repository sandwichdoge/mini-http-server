#include "http_server.h" //Master header

#define MAX_REQUEST_LEN 128*1024
#define CACHE_TABLE_SIZE 1024
#define WRITE_SZ 4096 * 8
#define READ_SZ 2048

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
//gcc -pg http_server.c http-request.c socket/serversocket.c socket/http-ssl.c mime/http-mimes.c fileops/fileops.c caching/caching.c caching/hashtable.c str-utils/str-utils.c -lpthread -lssl -lcrypto -pthread

typedef struct client_args {
    struct server_socket server_socket;
    int is_ssl;
    cache_file_t **cache_table;
    int cache_table_len;
} client_args;



int main()
{
    /*Load configs*/
    int config_errno = load_global_config();
    switch (config_errno) {
        case -1:
            printf("Fatal error in http.conf: No PATH parameter specified.\n");
            return config_errno;
        case -2:
            printf("Fatal error in http.conf: Invalid PATH parameter.\n");
            return config_errno;
        case -3:
            printf("Fatal error: cannot open http.conf\n");
            return config_errno;
    }

    chdir(SITEPATH); //change working dir to physical path of site

    /*Create sockets*/
    struct server_socket sock = create_server_socket(PORT);
    struct server_socket sock_ssl = create_server_socket(PORT_SSL);
    if (sock.fd < 0 || sock_ssl.fd < 0) {
        fprintf(stderr, "Error occurred during socket creation. Aborting.\n");
        return -4;
    }

    client_args args;
    pthread_t pthread[MAX_THREADS/2];

    /*Initialize SSL cert*/
    initialize_SSL();
    CTX = SSL_CTX_new(TLS_server_method());
    if (!CTX) {
        fprintf(stderr, "Error creating CTX.\n");
    }
    SSL_CTX_set_options(CTX, SSL_OP_SINGLE_DH_USE);
    //SSL_CTX_set_session_cache_mode(CTX, SSL_SESS_CACHE_SERVER); CACHE_SERVER is default setting, no need to set again

    SSL_CTX_use_certificate_file(CTX, CERT_PUB_KEY_FILE, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(CTX, CERT_PRIV_KEY_FILE, SSL_FILETYPE_PEM);
    if (!SSL_CTX_check_private_key(CTX)) {
        fprintf(stderr,"SSL FATAL ERROR: Can't open key files or private key does not match certificate public key.\n");
    }


    /*Start server*/
    printf("Started HTTP server on port %d and %d..\n", PORT, PORT_SSL);

    pid_t pid = fork(); //1 process for HTTP, 1 for HTTPS
    if (pid == -1) {
        perror("FATAL: Cannot create child processes.");
        return -5;
    }
    else if (pid == 0) {
        args.is_ssl = 1;
        args.server_socket = sock_ssl;
    }
    else {
        args.is_ssl = 0;
        args.server_socket = sock;
    }
    CACHE_TABLE = (cache_file_t**)table_create(CACHE_TABLE_SIZE);
    args.cache_table = CACHE_TABLE;
    args.cache_table_len = CACHE_TABLE_SIZE;

    signal(SIGINT, shutdown_server);
    signal(SIGTERM, shutdown_server);
    signal(SIGPIPE, SIG_IGN); //handle premature termination of connection from client

    /*CREATE n CHILD THREADS, n/2 FOR HTTP AND n/2 FOR HTTPS*/
    for (int i = 0; i < MAX_THREADS/2 - 1; ++i) {
        pthread_create(&pthread[i], NULL, conn_handler, (void*)&args); //create a thread for each new connection
    }

    conn_handler(&args); //handler for 2 parent threads

    //User will terminate server with Ctrl+C.

    return 0;
}


/*Handle an incoming connection
 *Return nothing
 *TODO: date, proper log*/
void *conn_handler(void *vargs)
{
    client_args *args = (client_args*) vargs;
    int client_fd = 0;//args->client_fd;
    struct server_socket server_socket = args->server_socket;
    int is_ssl = args->is_ssl;
    char buf[READ_SZ];
    char local_uri[2048];
    char decoded_uri[2048];
    char mime_type[128];
    char header[1024];
    char content_len[16];
    long sz;
    char *body_old = NULL;
    int saved_flock = 0;

    /*BIG LOOP HERE*/
    while (1) { //these declarations should be optimized by compiler anyway, it's ok to declare within loop
    int bytes_read = 0;
    int ssl_err = 0;
    int is_multipart = 0;
    char *request = NULL;
    char *_new_request;
    struct http_request *req = NULL;

    SSL *conn_SSL = NULL;

    //flush out buffers
    memset(buf, 0, sizeof(buf));
    memset(local_uri, 0, sizeof(local_uri));
    memset(decoded_uri, 0, sizeof(decoded_uri));
    memset(mime_type, 0, sizeof(mime_type));
    memset(header, 0, sizeof(header));
    memset(content_len, 0, sizeof(content_len));

    client_fd = accept(server_socket.fd, (struct sockaddr*)server_socket.handle, &server_socket.len);

    if (client_fd < 0) {
        fprintf(stderr, "Error accepting incoming connections.\n");
    }
    //printf("Connection established, fd:%d, thread:%ld\n", client_fd, pthread_self()); fflush(stdout);


    /*INITIALIZE SSL CONNECTION IF CONNECTED VIA HTTPS*/
    if (is_ssl) {
        saved_flock = fcntl(client_fd, F_GETFL);
        fcntl(client_fd, F_SETFL, saved_flock | O_NONBLOCK); //non-blocking IO for timeout measure
        
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
            usleep(10000); //check state every 0.01s
            --counter;
        } while (ssl_err == 2 && counter > 0); //ssl_err == 2 means handshake is still in process

        if (ssl_err) {
            fprintf(stderr, "Error accepting SSL handshake err:%d, fd:%d\n", ssl_err, client_fd); fflush(stderr);
            goto cleanup;
        }
    }

    /*READ HTTP REQUEST FROM CLIENT (INCLUDE HEADER+BODY)*/
    /*read data via TCP stream, append new data to heap
     *keep reading and allocating memory for new data until NULL or max request len reached
     *finally pass it it down to process_request() to get info about it (uri, method, body data)
     *if buf[sizeof(buf)] != 0, there's still more data*/
    int n = 0;
    int c = 5; //counter to handle SSL_ERROR_WANT_READ, after 5 retries terminate conn and clean up
    request = calloc(1, 1); //request points to new data, must be freed during cleanup
    if (request == NULL) {
        fprintf(stderr, "Out of memory.\n");
        return NULL;
    }

    do {
        //TODO: BOTTLENECK HERE
        if (is_ssl) { //https
            n = read_data_ssl(conn_SSL, buf, sizeof(buf));
        }
        else { //regular http without ssl
            n = read_data(client_fd, buf, sizeof(buf));
        }

        if (n < 0 && is_ssl) {
            ssl_err = SSL_get_error(conn_SSL, n);
            switch (ssl_err) {
                case SSL_ERROR_WANT_READ:
                    //More data from client to read
                    usleep(100000);
                    if (c-- == 0) goto cleanup;
                    continue;
                default:
                    fprintf(stderr, "Error %d reading data from client. fd: %d.\n", ssl_err, client_fd);
                    goto cleanup;
            }
        }

        if (n > 0) { //if there's data available
            bytes_read += n;
            _new_request = (char*)realloc(request, bytes_read + 1);
            if (_new_request == NULL) {
                fprintf(stderr, "Out of memory.\n"); fflush(stderr);
                goto cleanup; //out of memory
            }
            request = _new_request;
            memcpy(request + bytes_read - n, buf, n); //append data from tcp stream to *request
        }

        if (bytes_read >= MAX_REQUEST_LEN) {
            fprintf(stderr, "Request size exceeds limit.\n");
            break;
        }
    } while (buf[sizeof(buf)-1] != 0); //stop if request is larger than limit or reached NULL
    
    if (bytes_read == 0) {
        fprintf(stdin, "Received empty request.\n"); fflush(stdin);
        goto cleanup; //received empty request
    }

    /*PROCESS HEADER FROM CLIENT*/

    req = process_request(request);
    if (req == NULL) goto cleanup;

    /*HANDLE MULTIPART FILE UPLOAD*/
    //IF total data read < Content-Length, keep reading
    long total_read = req->body_len;
    long total_len = atoi(req->conn_len);

    if (total_read < total_len) {
        is_multipart = 1;
        body_old = req->body;
        req->body = calloc(total_len + 1, 1); //point body to new concatenated body, a hacky C thing
        memcpy(req->body, body_old, total_read); //concatenate old body to new body

        saved_flock = fcntl(client_fd, F_GETFL);
        fcntl(client_fd, F_SETFL, saved_flock | O_NONBLOCK);

        fd_set client_fd_monitor;
        FD_ZERO(&client_fd_monitor);
        FD_SET(client_fd, &client_fd_monitor);
        struct timeval tv; //timeout struct
        tv.tv_usec = 500000;

        while (total_read < total_len) {
            memset(buf, 0, sizeof(buf));
            if (select(client_fd +1, &client_fd_monitor, NULL, NULL, &tv) < 0) fprintf(stderr, "select() error.\n");

            if (is_ssl) { //https
                n = read_data_ssl(conn_SSL, buf, sizeof(buf));
            }
            else { //regular http without ssl
                n = read_data(client_fd, buf, sizeof(buf));
            }
            if (n <= 0) {
                if (errno != 11) perror("Error reading data.");
                break;
            }

            memcpy(req->body + total_read, buf, n);

            total_read += n;
        }
        req->body_len = total_len; //update body len

        saved_flock = fcntl(client_fd, F_GETFL);
        fcntl(client_fd, F_SETFL, saved_flock & ~O_NONBLOCK);
    }


    /*PROCESS URI (decode url, check privileges or if file exists)*/
    if (strcmp(req->URI, "/") == 0) strcpy(req->URI, HOMEPAGE); //when URI is "/", e.g. GET / HTTP/1.1
    decode_url(decoded_uri, req->URI); //decode url (%69ndex.html -> index.html)

    strncpy(local_uri, SITEPATH, SITEPATH_LEN); //allow only resources in site directory
    strncat(local_uri, decoded_uri, sizeof(decoded_uri) - SITEPATH_LEN); //local_uri: local path of requested resource

    if (!is_valid_method(req->method)) {
        http_send_error(client_fd, 400, conn_SSL); //then send 400 to client
        fprintf(stderr, "Unknown request method from client: %s.\n", req->method); fflush(stderr);
        goto cleanup;
    }

    if (file_exists(local_uri) < 0) { //if local resource doesn't exist
        fprintf(stderr, "Client requested non-existent resource %s.\n", local_uri); fflush(stderr);
        http_send_error(client_fd, 404, conn_SSL); //then send 404 to client
        goto cleanup;
    }
    /*
    if (file_readable(local_uri) < 0) { //local resource isn't read-accessible
        fprintf(stderr, "Local resource is inaccessible.\n"); fflush(stderr);
        http_send_error(client_fd, 403, conn_SSL);  //then send 403 to client
        goto cleanup;
    }*/

    //TODO: cookie, gzip content, handle other methods like PUT, HEAD, DELETE..
    
    /*SERVE DATA (EITHER STATIC PAGE OR INTERPRETED)*/
    //handle executable requests here
    char interpreter[1024] = ""; //path of interpreter program
    int is_interpretable = file_get_interpreter(local_uri, interpreter, sizeof(interpreter));

    if (is_interpretable > 0) { //CASE 1: uri is an executable file

        /*call interpreter, pass request body as argument*/
        char *p = local_uri;
    
        //NULL terminate body (only happens during GET with param requests), if request is POST, that means no body_len was returned
        if (req->body_len) req->body[req->body_len] = 0;

        /*handle env variables here
         *REQUEST_METHOD, CONTENT_TYPE (file upload), CONTENT_LENGTH, HTTP_COOKIE, PATH_INFO, QUERY_STRING (get w params),
         *SCRIPT_FILENAME*/
        
        env_vars_t e;
        env_vars_init(&e, req);

        char *env[] = {p, e.env_method, e.env_cookie, e.env_scriptpath, e.env_accept, e.env_querystr, e.env_conn_len, NULL};
        char *args[] = {interpreter, p, NULL};

        int ret_code;
        saved_flock = fcntl(client_fd, F_GETFL);
        fcntl(client_fd, F_SETFD, saved_flock | FD_CLOEXEC); //Turn off client_fd inheritance for child process
        char *data = system_output(args, env, req->body, req->body_len, &sz, &ret_code, 20000); //20s timeout on backend script
        
        env_vars_free(&e);
        if (is_multipart) free(req->body);

        if (ret_code < 0) { //if there's error in backend script, send err500 and skip sending returned data
            fprintf(stderr, "Internal error, code %d!", ret_code); fflush(stderr);
            http_send_error(client_fd, 500, conn_SSL);
            goto cleanup_output;
        }

        /*trim header parts from body*/
        char *doc;
        while (1) {
            doc = strstr(data, "\r\n\r\n");
            if (doc != NULL) break;
            doc = strstr(data, "\n\n");
            if (doc != NULL) break;
            doc = data; //last resort: backend did not generate a header
            break;
        }

        get_mime_type(mime_type, req->URI); //MIME type for response header
        sprintf(content_len, "%ld\n", sz - (doc - data)); //content-length for response header - equivalent to itoa(content_len)

        //at this point, data contains both header and body
        //now we generate header based on data returned from interpreter program
        if (generate_header(header, data, mime_type, content_len) < 0) {
            http_send_error(client_fd, 500, conn_SSL);
            goto cleanup_output;
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

        cleanup_output:
        free(data);
    }
    else if(is_interpretable == 0) { //CASE 2: uri is a static page
        if (CACHING_ENABLED) { /*TRY TO GET CONTENT FROM CACHE*/
            list_head_t *f = table_find((void**)args->cache_table, args->cache_table_len, local_uri);
            if (f == NULL) {  //file is not cached, cache it to memory and cache table
                f = cache_add_file(local_uri);
                if (f == NULL) {
                    fprintf(stderr, "Error caching server-side resource.\n");
                    http_send_error(client_fd, 500, conn_SSL); //go back and send data from disk instead?
                    goto cleanup;
                }
                table_add((void**)args->cache_table, args->cache_table_len, f);
            }

            //update node's last_accessed to current time.
            cache_file_t *p = (cache_file_t*)f;
            time(&p->last_accessed); 

            //generate header from cached media
            generate_header_static_from_cache(header, (cache_file_t*)f);

            
            //send header
            if (is_ssl) {
                send_data_ssl(conn_SSL, header, strlen(header));
            }
            else {
                send_data(client_fd, header, strlen(header));
            }
            //send body data
            serve_static_content_from_cache(client_fd, (cache_file_t*)f, conn_SSL);
        }
        else { /*READ CONTENT FROM DISK AND SERVE IT*/
            //generate header for static media
            generate_header_static_from_disk(header, local_uri);

            //send header
            if (is_ssl) {
                send_data_ssl(conn_SSL, header, strlen(header));
            }
            else {
                send_data(client_fd, header, strlen(header));
            }

            //send body data
            serve_static_content_from_disk(client_fd, local_uri, conn_SSL);
        }

    }
    else { //error reading requested data or system can't interpret requested script
        http_send_error(client_fd, 500, conn_SSL);
    }

    cleanup:
    //shutdown connection and free resources
    if (is_ssl) {
        saved_flock = fcntl(client_fd, F_GETFL);
        fcntl(client_fd, F_SETFL, saved_flock & ~O_NONBLOCK);
    }
    free(req); //request struct
    if (!ssl_err) free(request); //raw request
    if (is_ssl && conn_SSL != NULL) disconnect_SSL(conn_SSL, ssl_err); //will call additional SSL_free() if there's error (ssl_err != 0)
    sock_cleanup(client_fd);

    }

    return NULL;
}


/*Generate a header based on content of the body
 *body includes everything that backend script prints out, not just html content
 *header is the processed output
 *body is IMMUTABLE, do not attempt to modify it*/
int generate_header(char *header, char *body, char *mime_type, char *content_len)
{
    char buf[1024] = "";
    char user_defined_header[1024] = "";
    char httpver_final[32];
    char *doc_begin;
    char *n; //this will store linebreak char

    doc_begin = strstr(body, "\n<html");
    if (doc_begin == NULL) {
        doc_begin = body; //send whatever backend prints to client anyway, alternatively return -1
        strncpy(buf, body, sizeof(buf) - 1);
    }
    else {
        strncpy(buf, body, doc_begin - body);
    }

    lowercase(user_defined_header, buf, doc_begin - body);

    strcpy(header, user_defined_header);

    //define content-type if back-end does not
    char *cont_type = strstr(user_defined_header, "content-type: ");
    if (!cont_type) {
        str_insert(header, 0, "content-type: ");
        str_insert(header, 14,mime_type); //based on known MIME types
        str_insert(header, strlen("content-type: ") + strlen(mime_type) ,"; charset=utf-8\n");
    } 

    //define HTTP version if back-end doesn't specify
    char *httpver = strstr(user_defined_header, "http/");
    if (httpver && httpver - user_defined_header < 4) { //take into account unnecessary linefeeds from backend retards
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
    strip_trailing_lf(header, 2);
    strcat(header, "\ncontent-length: ");
    strcat(header, content_len);
    strcat(header, "server: mini-http-server\r\n\r\n");


    return 0;
}


/*Generate header for static media, read from disk*/
int generate_header_static_from_disk(char *header, char *local_uri)
{
    char content_len [16] = "";
    char mime_type[128] = "";

    get_mime_type(mime_type, local_uri); //MIME type for response header

    size_t sz = file_get_size(local_uri);
    sprintf(content_len, "%ld", sz);

    strcpy(header, "HTTP/1.1 200 OK\n");
    strcat(header, "Content-Type: ");
    strcat(header, mime_type);
    
    strcat(header, "\ncontent-length: ");
    strcat(header, content_len);

    strcat(header, "\r\n\r\n"); //mandatory blank line separating header
    return 0;
}


/*Generate header for static media, read from cached memory*/
int generate_header_static_from_cache(char *header, cache_file_t *f)
{
    char content_len [16] = "";
    char mime_type[128] = "";

    get_mime_type(mime_type, f->fname); //MIME type for response header

    sprintf(content_len, "%ld", f->sz);

    strcpy(header, "HTTP/1.1 200 OK\n");
    strcat(header, "Content-Type: ");
    strcat(header, mime_type);
    
    strcat(header, "\ncontent-length: ");
    strcat(header, content_len);

    strcat(header, "\r\n\r\n"); //mandatory blank line separating header
    return 0;
}


/*Return 0 if request method is not valid (any request that doesn't belong the methods set)*/
int is_valid_method(char *method)
{
    char *methods[] = {"GET", "POST", "HEAD", "PUT", "DELETE", "PATCH", "CONNECT", "OPTIONS"};
    int total = sizeof(methods) / sizeof(methods[0]);
    for (int i = 0; i < total; ++i) {
        if (strcmp(method, methods[i]) == 0) return 1;
    }
    return 0;
}


//Read local_uri and write to client socket pointed to by client_fd
void serve_static_content_from_disk(int client_fd, char *local_uri, SSL *conn_SSL)
{
    int bytes_read = 0; //bytes read from local resource
    size_t content_len = file_get_size(local_uri);
    char response[WRITE_SZ]; //buffer
    FILE *content_fd = fopen(local_uri, "r"); //get requested file content
    if (!content_fd) {
        fprintf(stderr, "Cannot open local file for reading: %s\n", local_uri);
        return;
    }
    int bytes_written = 0; //bytes sent to remote client
    bytes_read = 0;

    while (bytes_written < content_len) {
        memset(response, 0, sizeof(response));
        bytes_read = fread(response, 1, sizeof(response), content_fd); //content body
        if (conn_SSL) {
            bytes_written += send_data_ssl(conn_SSL, response, bytes_read);
        }
        else {
            bytes_written += send_data(client_fd, response, bytes_read);
        }
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

    fclose(content_fd);
}


/*Read data from cache and write to client socket pointed to by client_fd*/
void serve_static_content_from_cache(int client_fd, cache_file_t *f, SSL *conn_SSL)
{
    size_t content_len = f->sz;

    int bytes_written = 0; //bytes sent to remote client
    int bytes_to_write = WRITE_SZ; //send WRITE_SZ bytes at a time

    while (bytes_written < content_len) {
        if (bytes_written + bytes_to_write > content_len) {
            bytes_to_write = content_len - bytes_written;
        }
        
        if (conn_SSL) {
            bytes_written += send_data_ssl(conn_SSL, f->addr + bytes_written, bytes_to_write);
        }
        else {
            bytes_written += send_data(client_fd, f->addr + bytes_written, bytes_to_write);
        }
        if (bytes_written < 0) break; //error occurred, close fd and clean up;
    }

}


/*Send error to client*/
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


/*return path to interpreter in *out buffer.
*return code:
*0: file is not interpretable
*1: file is interpretable and interpreter exists on system
*-1: error reading file
*-2: interpretable file but interpreter does not exist on system or exec not allowed*/
int file_get_interpreter(char *path, char *out, size_t sz)
{
    //TODO: check allowed exec from global config, see if file is allowed execution
    char *ext = file_get_ext(path);
    list_head_t *h = table_find((void**)INTER_TABLE, INTER_TABLE_SZ, ext);
    if (h != NULL) {
        interpreter_t *it = (interpreter_t*)h;
        strcpy(out, it->path);
    }
    else {
        char buf[1024] = "";
        FILE *fd = fopen(path, "r"); if (!fd) return -1;
        if (fread(buf, 1, sizeof(buf), fd) == 0) return -1;
        fclose(fd);

        if (buf[0] != '#' || buf[1] != '!') return 0; //file is not for interpreting
        for (int i = 0; buf[i+2] != '\n' && i < sz; ++i) {
            out[i] = buf[i+2];
        }
    }    
    
    if (file_executable(path) < 0 || file_executable(out) < 0) {
        return -2; //no such interpreter on system or exec is not allowed
    }

    return 1;
}



/*Clean up and exit program*/
void shutdown_server()
{
    table_destroy((void**)CACHE_TABLE, CACHE_TABLE_SIZE, cache_remove_file);
    table_destroy((void**)INTER_TABLE, INTER_TABLE_SZ, remove_interpreter);
    shutdown_SSL();
    printf("Server stopped.\n");
    exit(0);
}


/*Initialize a struct to pass to interpreter's environment variables*/
int env_vars_init(env_vars_t *env, struct http_request *req)
{
    char *env_method = calloc(strlen("REQUEST_METHOD=") + strlen(req->method) + 1, 1);
    char *env_cookie = calloc(strlen("HTTP_COOKIE=") + strlen(req->cookie) + 1, 1);
    char *env_scriptpath = calloc(strlen("SCRIPT_FILENAME=") + strlen(req->URI) + 1, 1);
    char *env_scripturi = calloc(strlen("SCRIPT_URI=") + strlen(req->URI) + 1, 1);
    char *env_accept = calloc(strlen("HTTP_ACCEPT=") + strlen(req->accept) + 1, 1);
    char *env_querystr = calloc(strlen("QUERY_STRING=") + (req->query_str ? strlen(req->query_str) + 1 : 1), 1);
    char *env_conlen = calloc(strlen("CONTENT_LENGTH=") + strlen(req->conn_len) + 1, 1);

    strcpy(env_method, "REQUEST_METHOD="); strcat(env_method, req->method);
    strcpy(env_cookie, "HTTP_COOKIE="); strcat(env_cookie, req->cookie);
    strcpy(env_scriptpath, "SCRIPT_FILENAME="); strcat(env_scriptpath, req->URI);
    strcpy(env_scripturi, "SCRIPT_URI="); strcat(env_scripturi, req->URI);
    strcpy(env_querystr, "QUERY_STRING="); if (req->query_str) strcat(env_querystr, req->query_str);
    strcpy(env_accept, "HTTP_ACCEPT="); strcat(env_accept, req->accept);
    strcpy(env_conlen, "CONTENT_LENGTH="); strcat(env_conlen, req->conn_len);

    env->env_method = env_method;
    env->env_cookie = env_cookie;
    env->env_scriptpath = env_scriptpath;
    env->env_scripturi = env_scripturi;
    env->env_accept = env_accept;
    env->env_querystr = env_querystr;
    env->env_conn_len = env_conlen;

    return 0;    
}


/*Free up env variables*/
int env_vars_free(env_vars_t *env)
{
    free(env->env_accept); env->env_accept = NULL;
    free(env->env_cookie); env->env_cookie = NULL;
    free(env->env_method); env->env_method = NULL;
    free(env->env_querystr); env->env_querystr = NULL;
    free(env->env_scriptpath); env->env_scriptpath = NULL;
    free(env->env_querystr); env->env_querystr = NULL;
    free(env->env_conn_len); env->env_conn_len = NULL;

    return 0;
}

