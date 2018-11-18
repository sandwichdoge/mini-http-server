#include <stdio.h>
#include <string.h>
#include "hashtable.h"
#include "../fileops.h"


/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
cache_file_t* cache_add_file(char *path)
{
        cache_file_t *ret = (cache_file_t*)malloc(sizeof(cache_file_t));
        if (ret == NULL) return NULL;

        size_t filesz = file_get_size(path);
        if (filesz == 0) return NULL; //can't open file
        ret->addr = (char*)malloc(filesz);
        ret->fname = (char*)malloc(strlen(path)+1);
        if (ret->addr == NULL || ret->fname == NULL) return NULL; //out of memory

        strcpy(ret->fname, path);

        FILE *fd = fopen(path, "r");
        fread(ret->addr, 1, filesz, fd);
        fclose(fd);

        ret->sz = filesz;

        time(&ret->last_accessed);

        ret->next = NULL;

        return ret;
}
