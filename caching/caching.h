#include <stdio.h>
#include <string.h>
#include "hashtable.h"
#include "../fileops.h"


/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
cache_file_t* cache_file(char *path, cache_file_t **TABLE, int table_len)
{
        cache_file_t *ret = (cache_file_t*)malloc(sizeof(cache_file_t));

        size_t filesz = file_get_size(path);
        if (filesz == 0) return NULL; //can't open file
        ret->addr = (char*)malloc(filesz);
        ret->fname = (char*)malloc(strlen(path)+1);

        strcpy(ret->fname, path);

        FILE *fd = fopen(path, "r");
        fread(ret->addr, 1, filesz, fd);
        fclose(fd);

        ret->sz = filesz;

        time(&ret->last_accessed);

        ret->next = NULL;

        table_add(TABLE, table_len, ret);

        return ret;
}
