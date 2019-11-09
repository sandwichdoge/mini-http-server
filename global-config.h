#include "http_server.h"

#define INTER_TABLE_SZ 128

typedef struct interpreter_t {
    list_head_t HEAD;
    char *ext;
    char *path;
} interpreter_t;

/*Global variables*/
char SITEPATH[1024] = "";            //physical path of website on disk
int SITEPATH_LEN = 0;                  //len of SITEPATH
char HOMEPAGE[512] = "";           //default index page
char CERT_PUB_KEY_FILE[1024];  //pem file cert
char CERT_PRIV_KEY_FILE[1024]; //pem file cert
int PORT = 80;                              //default port 80
int PORT_SSL = 443;                     //default port for SSL is 443
int MAX_THREADS = 1024;            //maximum number of threads
int CACHING_ENABLED = 1;          //enable server-side caching
interpreter_t **INTER_TABLE;      //interpreter table
cache_file_t **CACHE_TABLE;       //cache table
SSL_CTX *CTX;                              //SSL cert


list_head_t *new_interpreter(char *ext, char *path)
{
    interpreter_t *p = malloc(sizeof(interpreter_t));
    p->ext = malloc(strlen(ext) + 1);
    p->path = malloc(strlen(path) + 1);
    strcpy(p->ext, ext);
    strcpy(p->path, path);

    p->HEAD.key = p->ext;
    p->HEAD.next = NULL;

    return &p->HEAD;
}


void remove_interpreter(list_head_t *head)
{
    interpreter_t *inter = (interpreter_t*)head;
    free(inter->ext);
    free(inter->path);
}


/*Load global configs, these configs will be visible to all child processes and threads*/
int load_global_config()
{
    char buf[4096];
    char *s;
    char *lnbreak;

    int fd = open("http.conf", O_RDONLY);
    if (fd < 0) return -3; //could not open http.conf
    read(fd, buf, sizeof(buf)); //read http.conf file into buf
    

    //SITEPATH: physical path of website
    s = strstr(buf, "PATH=");
    lnbreak = strstr(s, "\n");
    if (s == NULL) return -1; //no PATH config
    s += 5; //len of "PATH="
    memcpy(SITEPATH, s, lnbreak - s);
    if (!is_dir(SITEPATH)) return -2; //invalid SITEPATH
    SITEPATH_LEN = strnlen(SITEPATH, sizeof(SITEPATH));


    //PORT: port to listen on (default 80)
    s = strstr(buf, "PORT=");
    if (s == NULL) PORT = 80; //no PORT config, use default 80
    s += 5; //len of "PORT="
    PORT = atoi(s);

    //PORT for SSL (default 443)
    s = strstr(buf, "PORT_SSL=");
    if (s == NULL) PORT_SSL = 443; //no PORT config, use default 80
    s += 9; //len of "PORT_SSL="
    PORT_SSL = atoi(s);


    //HOMEPAGE: default index page
    s = strstr(buf, "HOME=");
    if (s == NULL) {
        strcpy(HOMEPAGE, "/index.html");
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 5;
        memcpy(HOMEPAGE, s, lnbreak - s);
    }
    

    //SSL stuff
    s = strstr(buf, "SSL_CERT_FILE_PEM="); //public key file path
    if (s == NULL) {
        CERT_PUB_KEY_FILE[0] = 0;
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 18; // len of "SSL_CERT_FILE_PEM="
        memcpy(CERT_PUB_KEY_FILE, s, lnbreak - s);
    }

    s = strstr(buf, "SSL_KEY_FILE_PEM="); //private key file path
    if (s == NULL) {
        CERT_PRIV_KEY_FILE[0] = 0;
    }
    else {
        lnbreak = strstr(s, "\n");
        s += 17; // len of "SSL_KEY_FILE_PEM="
        memcpy(CERT_PRIV_KEY_FILE, s, lnbreak - s);
    }


    //MAX_THREADS: maximum number of concurrent threads
    s = strstr(buf, "MAX_THREADS=");
    if (s == NULL) MAX_THREADS = 1024; //no config, use 1024
    s += strlen("MAX_THREADS="); //len of "MAX_THREADS="
    MAX_THREADS = atoi(s);


    //CACHING_ENABLED: enable server-side caching of static media
    s = strstr(buf, "CACHING_ENABLED=");
    if (s == NULL) CACHING_ENABLED = 1; //no config, use 1
    s += strlen("CACHING_ENABLED="); //len of "MAX_THREADS="
    CACHING_ENABLED = atoi(s);


    //INTERPRETER: custom backend interpreter
    char p[4096] = "";
    char *interp_conf = p;
    char ext[8];
    char inter[4096];
    INTER_TABLE = (interpreter_t**)table_create(INTER_TABLE_SZ);
    s = strstr(buf, "INTERPRETERS="); //public key file path
    if (s) {
        lnbreak = strstr(s, "\n");
        s += strlen("INTERPRETERS="); 
        memcpy(interp_conf, s, lnbreak - s);
    }
    int total = chr_count(interp_conf, ':');

    while (total--) {
        memset(ext, 0, sizeof(ext));
        memset(inter, 0, sizeof(inter));
        interp_conf = str_between(ext, interp_conf, '{', ':');
        interp_conf = str_between(inter, interp_conf, ':', '}');

        list_head_t *head = new_interpreter(ext, inter);
        table_add((void**)INTER_TABLE, INTER_TABLE_SZ, head);
    }

    //other configs below

    return 0;
}