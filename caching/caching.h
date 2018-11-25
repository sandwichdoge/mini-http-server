#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../fileops/fileops.h"

//struct packing polymorphism hack, key must always be the 1st element, *next is the 2nd #justCthings
typedef struct __attribute__((__packed__)) cache_file_t {
        char *key; //file name in this case
        struct cache_file_t *next;
        size_t sz;
        void *addr;
        time_t last_accessed;
} cache_file_t;


void** table_create(int len);
void* table_find(void **TABLE, int table_len, char *key);
int table_add(void **TABLE, int table_len, void *node);
int table_del_node(void **TABLE, int table_len, char *key, void (*free_func)(void*));
int table_destroy(void **TABLE, int len, void (*free_func)(void*));

/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
cache_file_t* cache_add_file(char *path);
void cache_remove_file(cache_file_t *f);
