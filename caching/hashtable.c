#include "caching.h"

cache_file_t** table_create(int len)
{
        return (cache_file_t**)calloc(len, sizeof(cache_file_t*));
}


/*Hash key/filename to gain index no*/
/*
static size_t hash(char *str, size_t max)
{
        size_t sum = 0;
        for (int i = 0; str[i]; i++) { //optimized
                sum += str[i] * (i + 1);
        }

        return sum % max;
}*/


/*Hash key/filename to gain index no*/
static size_t hashFNV(char *str, size_t max)
{
        size_t FNV_offset_basis = 0xcbf29ce484222325;
        size_t FNV_prime = 0x100000001b3;

        for (int i = 0; str[i]; i++) { //optimized
                FNV_offset_basis *= str[i] * FNV_prime;
                FNV_offset_basis ^= str[i];
        }

        return FNV_offset_basis % max;
}


/*free all allocated elements of nodes in table*/
int table_destroy(cache_file_t **TABLE, int len, int free_node)
{
        cache_file_t *prev;
        cache_file_t *head;
        for (int i = 0; i < len; i++) {
                head= TABLE[i];
                while (head != NULL) {
                        free(head->fname);
                        free(head->addr);
                        prev = head;
                        head = head->next;
                        if (free_node) free(prev);
                }
        }
        free(TABLE);

        return 0;
}



/*find node in table based on key string*/
cache_file_t* table_find(cache_file_t **TABLE, int table_len, char *key)
{
        size_t hkey = hashFNV(key, table_len);
        cache_file_t *node = TABLE[hkey];

        while (node != NULL) {
                if (strcmp(node->fname, key) == 0) {
                        time(&node->last_accessed);
                        return node;
                }
                node = node->next;
        }

        return NULL;
}


/*Add node into table*/
int table_add(cache_file_t **TABLE, int table_len, cache_file_t *node)
{
        size_t hkey = hashFNV(node->fname, table_len);

        cache_file_t *head = TABLE[hkey];
        if (head == NULL) {
                TABLE[hkey] = node;
                return 0;
        }
        else { /*if key already exists*/
                node->next = head;
                TABLE[hkey] = node;
        }

        return 0;
}


int table_del_node(cache_file_t **TABLE, int table_len, char *key)
{
        cache_file_t *f = table_find(TABLE, table_len, key);
        cache_file_t *next = f->next;
        size_t hkey = hashFNV(key, table_len);

        if (f == NULL) return -1; //node with key not found

        if (next) { //copy value of next node into current node, free next node
                f->addr = next->addr;
                f->fname = next->fname;
                f->last_accessed = next->last_accessed;
                f->sz = next->sz;
                f->next = next->next;
                cache_remove_file(next);
        }
        else { //found node is the only one of its line
                cache_remove_file(f);
                TABLE[hkey] = NULL;
        }

        return 0;
}