#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


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
int table_destroy(void **TABLE, int len, void (*free_func)(list_head_t*))
{
        list_head_t *prev;
        list_head_t *head;
        for (int i = 0; i < len; i++) {
                head= TABLE[i];
                while (head != NULL) {
                        if (free_func) free_func(head);
                        prev = head;
                        head = head->next;
                        free(prev);
                }
        }
        free(TABLE);

        return 0;
}


/*find node in table based on key string*/
list_head_t* table_find(void **TABLE, int table_len, char *key)
{
        size_t hkey = hashFNV(key, table_len);
        list_head_t *node = TABLE[hkey];
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
int table_add(void **TABLE, int table_len, list_head_t *HEAD)
{
        size_t hkey = hashFNV(HEAD->key, table_len);

        list_head_t *node = TABLE[hkey];
        if (node == NULL) {
                TABLE[hkey] = HEAD;
                return 0;
        }
        else { /*if key already exists*/
                HEAD->next = node;
                TABLE[hkey] = HEAD;
        }

        return 0;
}


/*Delete node from TABLE whose key matches parameter
 *free_func: function to be called on node to free up node resource*/
int table_del_node(void **TABLE, int table_len, char *key, void (*free_func)(list_head_t*))
{
        list_head_t *f = table_find(TABLE, table_len, key);
        if (f == NULL) return -1; //node with key not found

        size_t hkey = hashFNV(key, table_len);

        int count = 0;
        list_head_t *prev = f; 
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