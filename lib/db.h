#ifndef _DB_H_
#define _DB_H_

#include <stdbool.h>
#include <mysql/mysql.h>

extern MYSQL *conn;

bool connect_db();
void disconnect_db();

#endif
