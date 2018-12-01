#include "caching.h"

/*Cache file into memory.
 *Return address of file content buffer, file size and filename
 *Everything is stored on the heap*/
list_head_t* cache_add_file(char *path)
{
        cache_file_t *parent = malloc(sizeof(cache_file_t));
        if (parent == NULL) return NULL; //Out of memory

        size_t filesz = file_get_size(path);
        if (filesz == 0) return NULL; //can't open file

        parent->addr = (char*)malloc(filesz);
        parent->fname = (char*)malloc(strlen(path)+1);
        if (parent->addr == NULL || parent->fname == NULL) return NULL; //out of memory

        strcpy(parent->fname, path);

        FILE *fd = fopen(path, "r");
        fread(parent->addr, 1, filesz, fd);
        fclose(fd);

        parent->sz = filesz;

        time(&parent->last_accessed);

        /*init list_head_t struct for generic linked list*/
        parent->HEAD.key = parent->fname;
        parent->HEAD.next = NULL;

        return &parent->HEAD;
}


/*Remove file from memory buffer without freeing f*/
void cache_remove_file(list_head_t *fh)
{
        cache_file_t *f = (cache_file_t*)fh;
        free(f->addr);
        free(f->fname);

}