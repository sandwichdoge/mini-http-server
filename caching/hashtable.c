#include "hashtable.h"

cache_file_t** table_create(int len)
{
        return (cache_file_t**)calloc(len, sizeof(cache_file_t*));
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


/*Hash key/filename to gain index no*/
static size_t hash(char *str, size_t max)
{
        size_t sum = 0;
        for (int i = 0; str[i]; i++) { //optimized
                sum += str[i] * (i + 1);
        }

        return sum % max;
}


/*find node in table based on key string*/
cache_file_t* table_find(cache_file_t **TABLE, int table_len, char *key)
{
        size_t hkey = hash(key, table_len);
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
        size_t hkey = hash(node->fname, table_len);

        cache_file_t *head = TABLE[hkey];
        if (head == NULL) {
                TABLE[hkey] = node;
                return 0;
        }
        /*if key already exists*/
        node->next = head;
        TABLE[hkey] = node;

        return 0;
}