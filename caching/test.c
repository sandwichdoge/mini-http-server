#include <stdio.h>
#include "../fileops.h"
#include "caching.h"

//gcc -g test.c ../fileops.o

#define TABLESIZE 100

int main()
{
        cache_file_t **T = table_create(TABLESIZE);
        
        cache_file_t *f = cache_file("sample.txt", T, TABLESIZE);
        table_add(T, TABLESIZE, f);

        cache_file_t *result = table_find(T, TABLESIZE, "sample.txt");
        printf("%s\n", result->fname);

        return 0;
}