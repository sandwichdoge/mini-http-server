#include "http-ssl.h"


void initialize_SSL()
{
    SSL_load_error_strings();
    SSL_library_init();
    //SSL_CTX_set_cipher_list()
    OpenSSL_add_all_algorithms();
}

void shutdown_SSL()
{
    ERR_free_strings();
    EVP_cleanup();
}

void disconnect_SSL(SSL *conn_SSL)
{
    SSL_shutdown(conn_SSL);
    //SSL_free(conn_SSL);
}