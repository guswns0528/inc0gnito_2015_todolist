#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

#include "../lib/db.h"

static void logger_run();

pid_t logger_start()
{
    pid_t pid = fork();
    char buffer[65536];
    if (pid < 0)
        return -1;

    if (pid == 0)
    {
        logger_run();
        exit(0);
    }
    else
    {
        return pid;
    }
}

long get_message_count_from_db()
{
    mysql_query(conn, "select count(*) from messages");
    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);

    long count = 0;
    sscanf(row[0], "%ld", &count);

    mysql_free_result(result);
    return count;
}

void logger_run()
{
    while (1)
    {
        sleep(1);
        if (connect_db() == false)
            continue;
        long count = get_message_count_from_db();
        disconnect_db();
        if (count != 0)
        {
            int child = fork();
            if (child == 0)
            {
                if (connect_db() == false)
                {
                    return;
                }

                char message[512];
                if (mysql_query(conn, "select id, message, length(message) from messages order by id asc limit 0, 1"))
                {
                    disconnect_db();
                    return;
                }

                MYSQL_RES *sql_result = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(sql_result);
                if (row)
                {
                    int id;
                    sscanf(row[0], "%d", &id);
                    int message_length;
                    sscanf(row[2], "%d", &message_length);

                    memcpy(message, row[1], message_length);
                    printf("id: %d, message: %64s\n", id, message);

                    char query[128];
                    strcpy(query, "delete from messages where id = ");
                    strcat(query, row[0]);
                    printf("%s\n", query);

                    mysql_query(conn, query);
                }

                mysql_free_result(sql_result);

                disconnect_db();
                return;
            }
        }
    }
}
