#ifndef _LIST_H_
#define _LIST_H_

#include <stdbool.h>

struct list
{
    long sublist_size;
    void (*print)(struct list *);
    char *content;

    char **sublist;
    struct list *next;
};

void init_list(struct list ** l);
struct list* create_node();
void add_node(struct list *head, struct list *newnode);
struct list* get_node_with_index(struct list *head, long index);
bool delete_node_with_index(struct list* head, long index);
void change_sublist_with_index(struct list *node, long index, char *content);
void add_sublist(struct list *node, char *content);

#endif
