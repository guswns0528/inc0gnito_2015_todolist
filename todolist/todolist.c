#include "todolist.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/io.h"
#include "../lib/db.h"
#include "list.h"

static void initalize();
static void intro();
static void menu();
static long get_choice();

static bool print(long index);
static void print_all();
static void qna();
static void error() __attribute__((noreturn));

enum menu_choice
{
    menu_login = 1,
    menu_change_username,
    menu_add_new_todo,
    menu_change_todo,
    menu_delete_todo,
    menu_check_todo_done,
    menu_check_todo_undone,
    menu_add_sub_todo,
    menu_change_sub_todo,
    menu_delete_sub_todo,
    menu_show_list,
    menu_show_one,
    menu_qna,
    menu_quit,
    menu_invalid = -1
};

struct list *head;

#define NAME_LENGTH 3000
#define NAME_LEAK_LENGTH sizeof(local.name)

struct locals
{
    char name[NAME_LENGTH];
    const char *errormsg;
};

void todolist()
{
    initalize();
    intro();
    
    struct locals local;
    local.errormsg = NULL;
    /*
    char name[NAME_LENGTH];
    const char *errormsg = NULL;
    char error_buf[256];*/

    bool is_login = false;

    while (1)
    {
        if (local.errormsg != NULL)
        {
            // information leak
            // name is not null-padded.
            // And, errormsg does not reset.
            // So, when print name, address of errormsg will be printed.
            // It is clue of to find a image base address.
            printf(" [ERROR] %s\n", local.errormsg);
        }
        menu();
        long choice = get_choice();

        if (choice != menu_login && choice != menu_invalid && !is_login)
        {
            local.errormsg = "login first\n";
            continue;
        }

        bool quit = false;

        if (choice == menu_login)
        {
            write_string("Not implemented\n");
            write_string("Just set user name\n");
            long length = get_long();
            if (length > NAME_LEAK_LENGTH)
                length = NAME_LEAK_LENGTH;
            // not null-paded.
            read_exact_bytes(local.name, length);
            write_string("Hello ");
            write_string(local.name);
            is_login = true;
        }
        else if (choice == menu_change_username)
        {
            // not null-paded.
            long length = get_long();
            if (length > NAME_LEAK_LENGTH)
                length = NAME_LEAK_LENGTH;
            read_exact_bytes(local.name, length);
            write_string("Hello ");
            write_string(local.name);
        }
        else if (choice == menu_add_new_todo)
        {
            struct list *newnode = create_node();
            unsigned long content_legnth;
            write_string("input content length: ");
            read_bytes(&content_legnth, sizeof(content_legnth));

            char *content = malloc(sizeof(char) * (content_legnth + 1));
            read_bytes(content, content_legnth);
            content[content_legnth] = '\0';

            newnode->content = content;

            add_node(head, newnode);

            write_string("add new 'todo' success\n");
        }
        else if (choice == menu_change_todo)
        {
            write_string("input index of 'todo' you want to change its contents\n");
            unsigned long index;
            read_bytes(&index, sizeof(index));

            struct list *found = get_node_with_index(head, index);
            if (found == NULL)
            {
                local.errormsg = "Can't find 'todo' with the index\n";
            }
            else
            {
                write_string("Input new length of content\n");
                unsigned long length;
                read_bytes(&length, sizeof(length));

                char *new_content = malloc(sizeof(char) * (length + 1));
                write_string("Input new content\n");
                read_bytes(new_content, length);
                new_content[length] = '\0';

                free(found->content);
                found->content = new_content;

                write_string("Change content success\n");
            }
        }
        else if (choice == menu_delete_todo)
        {
            write_string("Input index of 'todo' you want to delete\n");
            unsigned long index;
            read_bytes(&index, sizeof(index));

            bool result = delete_node_with_index(head, index);
            if (result == false)
            {
                local.errormsg = "Index is invalid\n";
            }
            else
            {
                write_string("Delete todo success\n");
            }
        }
        else if (choice == menu_check_todo_done)
        {
            local.errormsg = "Sorry, not implemented\n";
        }
        else if (choice == menu_check_todo_undone)
        {
            local.errormsg = "Sorry, not implemented\n";
        }
        else if (choice == menu_add_sub_todo)
        {
            write_string("Input index of 'todo' you want to add sub todo\n");
            unsigned long index;
            read_bytes(&index, sizeof(index));

            struct list *found_node = get_node_with_index(head, index);
            if (found_node == NULL)
            {
                local.errormsg = "Can't find a 'todo' with the index\n";
                break;
            }

            write_string("Input length of sub todo\n");
            unsigned long length;
            read_bytes(&length, sizeof(length));

            char *subtodo = malloc(sizeof(char) * (length + 1));
            read_bytes(subtodo, length);
            subtodo[length] = '\0';

            add_sublist(found_node, subtodo);

            write_string("Add subtodo seccuess\n");
        }
        else if (choice == menu_change_sub_todo)
        {
            write_string("Input index of 'todo' you want to change sub todo\n");
            unsigned long node_index;
            read_bytes(&node_index, sizeof(node_index));

            struct list *node = get_node_with_index(head, node_index);
            if (node == NULL)
            {
                local.errormsg = "Can't find a 'todo' with the index\n";
                break;
            }

            write_string("Input index of sub todo you want to change its content\n");
            unsigned long index;
            read_bytes(&index, sizeof(index));

            write_string("Input length of new content\n");
            unsigned long length;
            read_bytes(&length, sizeof(length));

            char *content = malloc(sizeof(char) * (length + 1));
            write_string("Input new content\n");
            read_bytes(content, length);
            content[length] = '\0';

            change_sublist_with_index(node, index, content);
        }
        else if (choice == menu_delete_sub_todo)
        {
            local.errormsg = "Sorry, not implemented\n";
        }
        else if (choice == menu_show_list)
        {
            print_all();
        }
        else if (choice == menu_show_one)
        {
            long index;
            read_bytes(&index, sizeof(index));
            if (print(index) == false)
            {
                local.errormsg = "Index is invalid\n";
            }
        }
        else if (choice == menu_quit)
        {
            write_string("good bye\n");
            quit = true;
        }
        else if (choice == menu_qna)
        {
            qna();
        }
        else
        {
            error();
        }

        if (quit)
            break;
    }
}

void initalize()
{
    init_list(&head);
}

void intro()
{
    write_string("Todo-List manager v0.1a\n");
    write_string("Currently, data are not saved when quit.\n");
    write_string("It will be implemented at v0.2\n");
}

void menu()
{
    write_string("1. Login\n");
    write_string("2. Change username\n");
    write_string("3. Add todo\n");
    write_string("4. Change todo\n");
    write_string("5. Delete todo\n");
    write_string("6. Check todo done\n");
    write_string("7. Check todo undone\n");
    write_string("8. Add object\n");
    write_string("9. Change object\n");
    write_string("A. Delete object\n");
    write_string("B. Show all todos\n");
    write_string("C. Show todo\n");
    write_string("D. contact to admin\n");
    write_string("E. Quit\n");
}

long get_choice()
{
    char choice;
    if (read_bytes(&choice, sizeof(choice)) == false)
    {
        return (long)menu_invalid;
    }

    if (choice >= '1' && choice <= '9')
        return (long)(choice - '0');

    if (choice >= 'A' && choice <= 'E')
        return (long)(choice - 'A' + 10);

    return (long)menu_invalid;
}

bool print(long node_index)
{
    if (node_index < 1)
    {
        return false;
    }

    struct list *node = head->next;
    long index = 1;
    while (node != NULL && index < node_index)
    {
        node = node->next;
        index++;
    }

    if (index != node_index)
    {
        return false;
    }

    char buf[16];
    sprintf(buf, "%ld.\n", node_index);
    write_string(buf);
    node->print(node);

    return true;
}

void print_all()
{
    struct list *node = head->next;
    while (node != NULL)
    {
        node->print(node);
        node = node->next;
    }
}

void error()
{
    write_string("sorry, something goes wrong..\n");
    _exit(0);
}

bool qna_query(const char *message, long length)
{
    char query[2048];
    char *end;

    end = strcpy(query, "INSERT INTO messages(message) VALUES(");
    *end++ = '\'';
    end += mysql_real_escape_string(conn, end, message, length);
    *end++ = '\'';
    *end++ = ')';

    if (mysql_real_query(conn, query, (unsigned long)(end - query)))
    {
        return false;
    }

    return true;
}

void qna()
{
    if (!connect_db())
    {
        write_string("Sorry, temporary unavailable\n");
        return;
    }

    char message[512];
    write_string("Input message\n");
    int message_readed = read_bytes(message, sizeof(message) - 1);
    if (message_readed == -1)
    {
        error();
    }
    message[message_readed] = '\0';

    if (qna_query(message, message_readed) == false)
    {
        write_string("Sorry, failed to leave a message\n");
        return;
    }

    disconnect_db();
}
