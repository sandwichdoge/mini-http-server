#ifndef _http_request_h
#define _http_request_h
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "str-utils/str-utils.h"

struct http_request {
    char cookie[4096];
    char URI[2048];
    char accept[128];
    char httpver[16];
    char conn_type[16];
    char conn_len[16];
    char *query_str;
    char *body;
    long body_len;
    char method[8];
    int err;
};

//Return info about http request, must call cleanup_request() after use.
struct http_request *process_request(char *request);

//Convert URI code to real filename.
void decode_url(char *out, char *in);
#endif