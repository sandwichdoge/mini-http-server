#include "caching.h"

void** table_create(int len)
{
        return calloc(len, sizeof(void*));
}


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
int table_destroy(void **TABLE, int len, void (*free_func)(void*))
{
        void *prev;
        void *head;
        for (int i = 0; i < len; i++) {
                head= TABLE[i];
                while (head != NULL) {
                        free_func(head);
                        prev = head;
                        memcpy(&head, head + sizeof(void*), sizeof(void*));//head = head->next;
                        free(prev);
                }
        }
        free(TABLE);

        return 0;
}


/*find node in table based on key string*/
void* table_find(void **TABLE, int table_len, char *key)
{
        size_t hkey = hashFNV(key, table_len);
        void *node = TABLE[hkey];
        char *k;
        while (node != NULL) {
                memcpy(&k, node, sizeof(void*));
                if (strcmp(k, key) == 0) {
                        return node;
                }
                else {
                        memcpy(&node, node + sizeof(void*), sizeof(void*)); //node = node->next
                }
        }

        return NULL;
}


/*Add node into table*/
int table_add(void **TABLE, int table_len, void *node)
{
        char *k;
        memcpy(&k, node, sizeof(void*));

        size_t hkey = hashFNV(k, table_len);

        void *head = TABLE[hkey];
        if (head == NULL) {
                TABLE[hkey] = node;
                return 0;
        }
        else { /*if key already exists*/
                memcpy(node + sizeof(void*), &head, sizeof(void*));//node->next = head;
                TABLE[hkey] = node;
        }

        return 0;
}


//free_func: function to be called on node to free up node resource
int table_del_node(void **TABLE, int table_len, char *key, void (*free_func)(void*))
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
                free_func(f); //release cached memory & associated filename
                TABLE[hkey] = NULL;
        }
        else { //hash collision, match on inner nodes
                prev->next = f->next;
                free_func(f); //release cached memory & associated filename
        }
        free(f); //free allocated node

        return 0;
}