# mini-http-server
Primitive HTTP server in C.

TODO: handle more MIME types, privilege errors (403s), date and content-length.

- Compile
```
gcc http_server.c serversocket.c http-request.c fileops.c http-mimes.c -lpthread
```
