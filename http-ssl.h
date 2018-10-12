#ifndef _http_ssl_h_
#define _http_ssl_h_
#include <fcntl.h>
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void initialize_SSL();
void shutdown_SSL();
void disconnect_SSL(SSL *conn_SSL);

#endif