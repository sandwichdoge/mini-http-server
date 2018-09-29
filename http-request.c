#include "http-request.h"

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

    /*next line of http header
    i.e. accept-encoding, user-agent, referer, accept-language*/
    char *header_fields = request+method_len+uri_len+httpver_len+2;
    //printf("%s\n", request);
    //fflush(stdout);
    return ret;
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