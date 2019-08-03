#ifndef _CONF_H
#define _CONF_H

#include "defs.h"

typedef struct _st_conf{

	//alert
	char *alert_bind_host;
	int alert_bind_port;

	//mysql
	char *mysql_host;
	int mysql_port;
	char *mysql_db;
	char *mysql_user;
	char *mysql_passwd;
	int mysql_connect_delay;
	int mysql_connect_tout;
	int mysql_read_tout;
	int mysql_write_tout;
	int mysql_retry_count;
	int mysql_retry_sleep;

	//clients
	int max_clients;
	int max_clients_per_ip;

	//storage
	char *storage_path;
	char *configs_path;

	//system
	char *user;

	//threads
	int thread_stacksize;

	int pk_read_tout;
	int pk_write_tout;

} st_conf, *pst_conf;

typedef struct _st_stream_conf{

	char *save_path;
	char *rtsp_url;
	int min_run_time;

} st_stream_conf, *pst_stream_conf;

typedef struct _st_conf_opt_str{
	const char *name;
	char **value;
	const char *default_value;
} st_conf_opt_str, *pst_conf_opt_str;

typedef struct _st_conf_opt_int{
	const char *name;
	int *value;
	const int default_value;
} st_conf_opt_int, *pst_conf_opt_int;

int conf_init(const char *fname, st_conf_opt_str *conf_opt_str, st_conf_opt_int *conf_opt_int);
int conf_init_nvr(const char *fname);
int conf_init_stream(const char *fname, st_stream_conf *conf);

#endif
