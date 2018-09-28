# mini-http-server
Primitive HTTP server in C.

Only server static html pages so far.

TODO: handle MIME types, privilege errors (403s), date and content-length

- Compile
```
gcc -lpthread http_server.c
```
