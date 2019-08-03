#ifndef _STRUCT_H
#define _STRUCT_H

#include <pthread.h>
#include <mysql/mysql.h>
#include <videostreamer.h>

#include "conf.h"
#include "defs.h"

#pragma pack(push, 1)
typedef struct _st_alert_header_20{
	unsigned int ui1;
	unsigned int ui2;
	unsigned int ui3;
	unsigned int ui4;
	unsigned int data_size;
} st_alert_header, *pst_alert_header;
#pragma pack(pop)

//alert
typedef struct _st_alert_data{
	char *json;
	const char *address;
	const char *serial;
	const char *status;
	const char *event;
	const char *type;
} st_alert_data, *pst_alert_data;

//cl
typedef struct _st_client{
	pthread_t thread_id;

	//connection
	int sd;
	unsigned int ip;
	char ip_str[INET_ADDR_STR_LEN];
	int port;

	//protocol
	st_alert_header alert_header;
	st_alert_data alert_data;
	void *pk_data;

	void *prev;
	void *next;
} st_client, *pst_client;

//stream
typedef struct _st_stream{
	pthread_t thread_id;

	//connection
	unsigned int ip;
	char ip_str[INET_ADDR_STR_LEN];

	//conf
	st_stream_conf conf;

	//stream
	time_t updated;
	char dst[MAX_PATH];
	struct VSInput *input;
	struct VSOutput *output;

	void *prev;
	void *next;
} st_stream, *pst_stream;

//mysql
typedef struct _st_mysql{
	int state;

	MYSQL *mysql;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *mysql_db;

	time_t try_t;
	pthread_mutex_t mysql_mutex;
	unsigned long thread_id;

	char *query;
	int query_type;
	int query_cnt;
	int query_max;

	my_ulonglong insert_id;
	my_ulonglong affected_rows;
	my_ulonglong num_rows;
	unsigned int num_fields;

} st_mysql, *pst_mysql;

//global
typedef struct _st_g{

	//conf
	st_conf conf;

	//clients
	int alert_sd;
	st_client *clients;
	pthread_mutex_t clients_mutex;

	//streams
	st_stream *streams;
	pthread_mutex_t streams_mutex;

	//threads
	pthread_attr_t thread_attr;

	//db
	st_mysql db;

} st_g, *pst_g;

#endif
