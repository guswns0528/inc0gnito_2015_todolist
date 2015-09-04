#include "list.h"
#include "../lib/io.h"

#include <stdlib.h>

static void print_without_sublist(struct list *node);
static void print_with_sublist(struct list *node);

void init_list(struct list **l)
{
    *l = malloc(sizeof(struct list));
    struct list* head = *l;
    head->sublist_size = 0;
    head->content = NULL;

    head->sublist = malloc(0);
    head->next = NULL;

    head->print = NULL;
}

struct list* create_node()
{
    struct list* result = malloc(sizeof(struct list));

    result->sublist_size = 0;
    result->print = print_without_sublist;
    result->content = NULL;
    result->sublist = NULL;

    result->next = NULL;

    return result;
}

void add_node(struct list *head, struct list *newnode)
{
    while (head->next != NULL)
    {
        head = head->next;
    }

    head->next = newnode;
}

struct list* get_node_with_index(struct list *head, long index)
{
    if (index < 1)
        return NULL;

    struct list *node = head->next;
    while (node != NULL && index > 1)
    {
        node = node->next;
        index--;
    }

    return node;
}

bool delete_node_with_index(struct list* head, long index)
{
    struct list *prev = head;
    while (head->next != NULL && index > 0)
    {
        prev = head;
        head = head->next;
        index--;
    }

    if (index == 0 && head != NULL)
    {
        prev->next = head->next;

        if (head->sublist != NULL)
        {
            for (long i = 0; i < head->sublist_size; i++)
            {
                free(head->sublist[i]);
            }
            free(head->sublist);
        }
        free(head);

        return true;
    }

    return false;
}

void change_sublist_with_index(struct list *node, long index, char *content)
{
    if (index >= node->sublist_size)
        return;

    if (node->sublist == NULL)
        return;

    // integer overflow
    // It's harmful
    node->sublist[index] = content;
}

void add_sublist(struct list *node, char *content)
{
    if (node->sublist_size == 0)
    {
        node->sublist = malloc(sizeof(char *));
        node->print = print_with_sublist;
    }
    else
    {
        node->sublist = realloc(node->sublist, sizeof(char *) * (node->sublist_size + 1));
    }

    node->sublist[node->sublist_size] = content;
    node->sublist_size++;
}

void print_without_sublist(struct list *node)
{
    write_string(node->content);
    write_string("\n");
}

void print_with_sublist(struct list *node)
{
    write_string(node->content);
    write_string("\n");

    for (long i = 0; i < node->sublist_size; i++)
    {
        write_string(" - ");
        write_string(node->sublist[i]);
        write_string("\n");
    }
}
