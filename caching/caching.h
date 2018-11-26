#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../hashtable/hashtable.h"
#include "../fileops/fileops.h"


typedef struct cache_file_t {
        list_head_t HEAD;
        char *fname;
        size_t sz;
        void *addr;
        time_t last_accessed;
} cache_file_t;


/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
list_head_t* cache_add_file(char *path);
void cache_remove_file(list_head_t *f);
