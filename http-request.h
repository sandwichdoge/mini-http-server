#ifndef _http_request_h
#define _http_request_h
#include <string.h>
#include <stdlib.h>
struct http_request {
    char method[8];
    char URI[1024];
    char httpver[16];
};

struct http_request process_request(char *request);
void decode_url(char *out, char *url);
#endif