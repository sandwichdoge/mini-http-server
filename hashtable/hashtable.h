#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_
/*Generic linked list
 *We can attach anything after the head, functions operate only on head
 *Courtesy to Linux kernel list implementation*/
typedef struct list_head_t {
        char *key;
        struct list_head_t *next;
} list_head_t;


void** table_create(int len);
list_head_t* table_find(void **TABLE, int table_len, char *key);
int table_add(void **TABLE, int table_len, list_head_t *node);
int table_del_node(void **TABLE, int table_len, char *key, void (*free_func)(list_head_t*));
int table_destroy(void **TABLE, int len, void (*free_func)(list_head_t*));

#endif