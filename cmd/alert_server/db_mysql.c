#include <stdio.h>
#include "defs.h"
#include <string.h>
#include <mysql/mysql.h>

#include "db_mysql.h"
#include "nvr.h"
#include "log.h"
#include "structs.h"

extern st_g g;

void init_db(int conn){
	memset((char*)&g.db, 0, sizeof(g.db));
	if(!(g.db.mysql=mysql_init(NULL))){
		_LOG_ERR("mysql_init: g.db");
		die(0);
	}
	g.db.mysql_db = g.conf.mysql_db;

	if(conn)
		db_mysql_connect(&g.db);
}

int db_mysql_connect(st_mysql *db){
	time_t try_t, wait_t;
	unsigned long thread_id;

	if(db->state & DB_MYSQL_CONNECTED){
		thread_id = mysql_thread_id(db->mysql);
		if(!mysql_ping(db->mysql)){
			if(thread_id != db->thread_id){
				db->thread_id = thread_id;
				_LOG_WRN("mysql_ping: thread_id=%lu reconnected", db->thread_id);
			}

			return 0;
		}

		db->state&= ~DB_MYSQL_CONNECTED;
		_LOG_DB_WRN(db->mysql, "mysql_ping: failed, try reconnect");
	}

	//flood connect 4eck
	try_t = time(NULL);
	wait_t = try_t - db->try_t;
	if(wait_t < g.conf.mysql_connect_delay){
		_LOG_WRN("flood protection: mysql_connect_delay=%d, sleep=%lu", g.conf.mysql_connect_delay, g.conf.mysql_connect_delay-wait_t);
		return g.conf.mysql_connect_delay-wait_t;
	}

	//set time outs
	_LOG_DBG("try mysql_options: mysql_connect_tout=%d, mysql_read_tout=%d, mysql_write_tout=%d", g.conf.mysql_connect_tout, g.conf.mysql_read_tout, g.conf.mysql_write_tout);
	if(mysql_options(db->mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const void *)&g.conf.mysql_connect_tout)
	|| mysql_options(db->mysql, MYSQL_OPT_READ_TIMEOUT, (const void *)&g.conf.mysql_read_tout)
	|| mysql_options(db->mysql, MYSQL_OPT_WRITE_TIMEOUT, (const void *)&g.conf.mysql_write_tout)){
		_LOG_DB_ERR(db->mysql, "mysql_options: mysql_connect_tout=%d, mysql_read_tout=%d, mysql_write_tout=%d", g.conf.mysql_connect_tout, g.conf.mysql_read_tout, g.conf.mysql_write_tout);
		return -1;
	}

	//try now
	_LOG_DBG("try mysql_real_connect: host=%s, user=%s, db=%s", g.conf.mysql_host, g.conf.mysql_user, db->mysql_db);
	db->try_t = try_t;
	if(!mysql_real_connect(db->mysql, g.conf.mysql_host, g.conf.mysql_user, g.conf.mysql_passwd, db->mysql_db, g.conf.mysql_port, NULL, 0)){
		_LOG_DB_ERR(db->mysql, "mysql_real_connect: failed");
		return -1;
	}

	//done here
	db->state|= DB_MYSQL_CONNECTED;
	db->thread_id = mysql_thread_id(db->mysql);

	_LOG_DBG("thread_id=%lu connected to db=%s", db->thread_id, db->mysql_db);

	return 0;
}

int db_mysql_query(st_mysql *db){
	int wait_t;

	if((db->state & DB_MYSQL_PRE_QUERY_CHECK) && (db->query_cnt <= 0 || db->query_cnt >= db->query_max)){
		_LOG_ERR("pre query check: query_cnt=%d, query_max=%d", db->query_cnt, db->query_max);
		return -1;
	}

	if((wait_t=db_mysql_connect(db)))
		return wait_t;

	_LOG_DBG("query=%s", db->query);

	if(mysql_query(db->mysql, db->query)){
		_LOG_DB_ERR(db->mysql, "mysql_query: failed, query=%s", db->query);
		return -1;
	}

	switch(db->query_type){
		case DB_MYSQL_QTYPE_SELECT:

			//clean up
			if(db->res){
				mysql_free_result(db->res);
				db->res = NULL;
			}

			//store_result for select
			if(!(db->res=mysql_store_result(db->mysql))){

				//query error
				_LOG_DB_ERR(db->mysql, "mysql_store_result: failed, query=%s", db->query);
				return -1;
			}

			db->num_rows = mysql_num_rows(db->res);
			db->num_fields = mysql_num_fields(db->res);

			_LOG_DBG("num_rows=%lu, num_fields=%lu", (unsigned long)db->num_rows, (unsigned long)db->num_fields);
			break;

		case DB_MYSQL_QTYPE_UPDATE:
			db->affected_rows = mysql_affected_rows(db->mysql);

			_LOG_DBG("affected_rows=%lu", (unsigned long)db->affected_rows);
			break;

		case DB_MYSQL_QTYPE_INSERT:
			db->insert_id = mysql_insert_id(db->mysql);

			_LOG_DBG("insert_id=%lu", (unsigned long)db->insert_id);
			break;
	}

	return 0;
}
