#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //read()/write(), usleep()
#include <pthread.h> //multi-connection
#include <fcntl.h>  //to read html files
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "socket/serversocket.h"
#include "socket/http-ssl.h"
#include "http-request.h"
#include "fileops/fileops.h"
#include "mime/http-mimes.h"
#include "sysout.h"
#include "str-utils/casing.h" //uppercase() and lowercase()
#include "str-utils/str-utils.h"
#include "caching/caching.h"
#include "global-config.h"


typedef struct env_vars_t {
    char *env_method;
    char *env_cookie;
    char *env_scriptpath;
    char *env_scripturi;
    char *env_accept;
    char *env_querystr;
    char *env_conn_len;
} env_vars_t;


void *conn_handler(void *fd);
int is_valid_method(char *method);
int env_vars_init(env_vars_t *env, struct http_request *req);
int env_vars_free(env_vars_t *env);
int generate_header_interpreted(char *header, char *body, char *mime_type, char *content_len);
int generate_header_static_from_disk(char *header, char *local_uri);
int generate_header_static_from_cache(char *header, cache_file_t *f);
int file_get_interpreter(char *path, char *out, size_t sz);
void serve_static_content_from_disk(int client_fd, char *local_uri, SSL *conn_SSL);
void serve_static_content_from_cache(int client_fd, cache_file_t *f, SSL *conn_SSL);
void http_send_error(int client_fd, int errcode, SSL *conn_SSL);

void shutdown_server();

#endif