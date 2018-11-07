#include "http-request.h"

//return info about client request like method, uri, http version, etc.
//CALLER IS RESPONSIBLE FOR FREEING DATA RETURNED FROM THIS FUNCTION
struct http_request *process_request(char *request)
{
    struct http_request *ret = malloc(sizeof(struct http_request));
    int method_len = 0;
    int uri_len = 0;
    int is_GET_with_params = 0;
    char *n; //this will contain linebreak char

    /*HEADER SECTION*/
    memset(ret->URI, 0, sizeof(ret->URI));
    memset(ret->method, 0, sizeof(ret->method));
    memset(ret->httpver, 0, sizeof(ret->httpver));
    memset(ret->conn_type, 0, sizeof(ret->conn_type));
    memset(ret->cookie, 0, sizeof(ret->cookie));
    ret->body_len = 0;

    //method; method_len = len of method string
    for (method_len; request[method_len] != ' ' && method_len <= sizeof(ret->method); ++method_len);
    memcpy(ret->method, request, method_len);
    ret->method[method_len] = 0; //NULL terminate

    //requested URI
    char *URI = strchr(request, ' ');
    char *URI_end = NULL;
    if (URI) {
        URI_end = strchrln(URI + 1, '?');
        if (URI_end) { //somebody tries to send a form with GET method
            //handle it anyway?
            is_GET_with_params = 1;
        }
        else {
            URI_end = strchr(URI + 1, ' ');
        }
    }
    if (URI && URI_end) {
        uri_len = URI_end - URI - 1;
        memcpy(ret->URI, URI + 1, uri_len);
    }
    else {
        strcpy(ret->URI, "/");
    }

    //Protocol/ HTTP version
    char *httpver;
    if (!URI) { //no uri or anything specified from client,
        httpver = NULL; //shouldn't happen unless client sends niggerlicious requests like just 3 chars 'GET' with no additional info
    }
    else {
        httpver = strstr(URI + uri_len, "HTTP/");
    }
    if (httpver) {
        strncpy(ret->httpver, httpver, strchr(httpver, '\n') - httpver);
    }
    else {
        strcpy(ret->httpver, "HTTP/1.1"); //use default HTTP ver
    }

    //Accept
    char *accept;
    accept = strstr(URI + uri_len, "Accept: ");
    if (accept) {
        strncpy(ret->accept, accept + strlen("Accept: "), strchr(accept, '\n') - accept - strlen("Accept: "));
    }
    else {
        strcpy(ret->accept, "*/*"); //any mime type
    }

    //Connection type (keep-alive/close)
    char *conn_type;
    conn_type = strstr(URI + uri_len, "Connection: ");
    if (conn_type) {
        strncpy(ret->conn_type, conn_type + strlen("Connection: "), strchr(conn_type, '\n') - conn_type - strlen("Connection: "));
    }
    else {
        strcpy(ret->conn_type, "close");
    }

    //Cookie
    char *cookie = strstr(request, "Cookie: "); 
    if (cookie) {
        cookie += 8; //8 = len of "Cookie: "
        n = strchr(cookie, '\n');
        if (n) {
            int cookie_len = n - cookie;
            strncpy(ret->cookie, cookie, cookie_len);
        }
    }
    /*next line of http header
    i.e. accept-encoding, user-agent, referer, accept-language*/


    /*BODY SECTION*/
    char *body;
    if (is_GET_with_params) { //handle GET form submit.py?age=18
        ret->body = URI_end + 1; //point body to end of local resource path (submit.py?), URI_end points to '?' in this case
        request[URI + uri_len - request] = 0; //terminate the part after uri (submit.py?age=18NULL)
        ret->body_len = strchr(ret->body, ' ') - (ret->body); //URI_end+1 is start of argument fields
    }
    else {
        body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4; //strip preceding blank line
            ret->body = body;
            //ret->body_len = strlen(body);
        }
    }
    
    return ret;
}


//Fast url decoder %69ndex.html => index.html
void decode_url(char *out, char *in)
{
    static const char tbl[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
         0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
    };
    char c, v1, v2;
    char *beg=out;
    if(in != NULL) {
        while((c=*in++) != '\0') {
            if(c == '%') {
                if(!(v1=*in++) || (v1=tbl[(unsigned char)v1])<0 || !(v2=*in++) || (v2=tbl[(unsigned char)v2])<0) {
                    *beg = '\0';
                    break;//return -1;
                }
                c = (v1<<4)|v2;
            }
            *out++ = c;
        }
    }
    *out = '\0';
    return;
}