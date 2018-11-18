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
        cache_file_t *f2 = cache_file("sample2.txt", T, TABLESIZE);
        table_add(T, TABLESIZE, f2);


        cache_file_t *result = table_find(T, TABLESIZE, "sample2.txt");
        if (result) printf("%d\n", result->last_accessed);

        table_destroy(T, TABLESIZE, 1);


        return 0;
}