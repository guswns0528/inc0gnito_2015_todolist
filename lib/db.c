#include "../config.h"
#include "db.h"

#define NULL (void*)0

static MYSQL mysql_connection;
MYSQL *conn = NULL;

bool connect_db()
{
    conn = mysql_real_connect(&mysql_connection, "localhost", mysql_user, mysql_pass, mysql_dbname, 3306, (char *)NULL, 0);

    if (conn == NULL)
        return false;
    return true;
}

void disconnect_db()
{
    mysql_close(conn);
}
