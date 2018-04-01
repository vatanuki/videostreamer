#ifndef _DB_MYSQL_H
#define _DB_MYSQL_H

#include "defs.h"
#include "structs.h"

void init_db(int conn);
void init_db_thread(int conn);
int db_mysql_connect(st_mysql *db);
int db_mysql_query(st_mysql *db);

#endif
