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
                        free(head->key);
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
                if (strcmp(node->key, key) == 0) {
                        return node;
                }
                else {
                        node = node->next;
                }
        }

        return NULL;
}


/*Add node into table*/
int table_add(cache_file_t **TABLE, int table_len, cache_file_t *node)
{
        size_t hkey = hashFNV(node->key, table_len);

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


//free_func: function to be called on node to free up node resource
int table_del_node(cache_file_t **TABLE, int table_len, char *key, void (*free_func)(void*))
{
        cache_file_t *f = table_find(TABLE, table_len, key);
        if (f == NULL) return -1; //node with key not found

        size_t hkey = hashFNV(key, table_len);

        int count = 0;
        cache_file_t *prev = f; 
        while (strcmp(key, f->key) != 0) { //traverse linked list until keys match
                prev = f;
                f = f->next;
                count++;
        }

        if (count == 0) { //match on first node
                free_func(f);
                TABLE[hkey] = NULL;
        }
        else { //hash collision, match on inner nodes
                prev->next = f->next;
                free_func(f);
        }
        
        return 0;
}