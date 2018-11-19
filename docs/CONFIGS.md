Configs are loaded upon starting up server.

Parameters:
+ PATH              : Physical path for website on disk.
+ PORT              : Port for HTTP connections.
+ PORT_SSL          : Port for HTTPS connections.
+ HOME              : Default homepage, must be preceeded by '/' symbol.
+ SSL_CERT_FILE_PEM : Physical path to .pem TLS public cert file.
+ SSL_KEY_FILE_PEM  : Physical path to .pem TLS private key file.
+ MAX_THREADS       : Maximum number of concurrent threads server can run.
+ CACHING_ENABLED   : Enable server-side caching on static media to avoid I/O bottleneck.
