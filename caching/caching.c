#include "caching.h"

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
        ret->key = (char*)malloc(strlen(path)+1);
        if (ret->addr == NULL || ret->key == NULL) return NULL; //out of memory

        strcpy(ret->key, path);

        FILE *fd = fopen(path, "r");
        fread(ret->addr, 1, filesz, fd);
        fclose(fd);

        ret->sz = filesz;

        time(&ret->last_accessed);

        ret->next = NULL;

        return ret;
}


/*Remove file from memory buffer without freeing f*/
void cache_remove_file(cache_file_t *f)
{
        free(f->addr);
        free(f->key);

}