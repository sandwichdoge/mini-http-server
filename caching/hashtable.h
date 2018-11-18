#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_
#include <stdlib.h>
#include <string.h>
#include <time.h>


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

#endif