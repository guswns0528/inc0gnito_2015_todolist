#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "lib/error.h"

#include "logger/logger.h"
#include "network/network.h"
#include "permission/permission.h"
#include "todolist/todolist.h"
#include "config.h"

#define PORT 32323

static void initialize();
static bool load_mysql_info();

int main(void)
{
    initialize();
    int server_socket = create_socket(PORT);
    if (server_socket == -1)
    {
        exit_on_error("Can't create a socket\n");
    }

    printf("ready to serve!\n");

    service_forever(server_socket, todolist);

    return 0;
}

void initialize()
{
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        exit_on_error("signal(SIGCHLD, SIG_IGN)\n");
    }

    if (!load_mysql_info())
    {
        exit_on_error("failed to load mysql account infomation\n");
    }

    if (false && drop_privileges() == false)
    {
        exit_on_error("failed to set privileges\n");
    }

    if (logger_start() == -1)
    {
        exit_on_error("Can't create a logger process\n");
    }
}

bool load_mysql_info()
{
    FILE *fp = fopen("./mysql_info", "rt");
    if (!fp)
    {
        return false;
    }

    if (fgets(mysql_user, sizeof(mysql_user), fp) == NULL)
    {
        goto err;
    }

    if (fgets(mysql_pass, sizeof(mysql_pass), fp) == NULL)
    {
        goto err;
    }

    if (fgets(mysql_dbname, sizeof(mysql_dbname), fp) == NULL)
    {
        goto err;
    }

    mysql_user[strlen(mysql_user) - 1] = '\0';
    mysql_pass[strlen(mysql_pass) - 1] = '\0';
    mysql_dbname[strlen(mysql_dbname) - 1] = '\0';

    return true;
err:
    fclose(fp);
    return false;
}
