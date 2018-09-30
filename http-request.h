#ifndef _http_request_h
#define _http_request_h
#include <string.h>
#include <stdlib.h>
struct http_request {
    char method[8];
    char URI[1024];
    char httpver[16];
    char *body;
    int err;
};

//Return info about http request, must call cleanup_request() after use.
struct http_request process_request(char *request);

//Cleanup http request struct.
int cleanup_request(struct http_request);

//Convert URI code to real filename.
void decode_url(char *out, char *url);
#endif