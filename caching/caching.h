#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../fileops/fileops.h"

typedef struct cache_file_t {
        char *fname;
        size_t sz;
        void *addr;
        time_t last_accessed;
        struct cache_file_t *next;
} cache_file_t;


cache_file_t** table_create(int len);
cache_file_t* table_find(cache_file_t **TABLE, int table_len, char *key);
int table_add(cache_file_t **TABLE, int table_len, cache_file_t *node);
int table_destroy(cache_file_t **TABLE, int len, int free_node);
int table_del_node(cache_file_t **TABLE, int table_len, char *key);

/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
cache_file_t* cache_add_file(char *path);
int cache_remove_file(cache_file_t *f);
