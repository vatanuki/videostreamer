#include <stdio.h>
#include "defs.h"
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "str.h"
#include "conf.h"
#include "structs.h"
#include "streams.h"

extern st_g g;

int conf_init(const char *fname, st_conf_opt_str *conf_opt_str, st_conf_opt_int *conf_opt_int){
	FILE *fp;
	char buf[CONF_STR_SIZE], *key, *val;
	pst_conf_opt_str opt_str;
	pst_conf_opt_int opt_int;

	//open config
	if(!(fp=fopen(fname, "r"))){
		_LOG_ERR("error open config file=%s | %m", fname);
		return -1;
	}

	//str defs
	opt_str = (pst_conf_opt_str)conf_opt_str;
	while(opt_str->name){
		*(opt_str->value) = opt_str->default_value ? strdup((char*)opt_str->default_value) : NULL;
		_LOG_DBG("default config str: %s = %s", opt_str->name, *(opt_str->value));
		opt_str++;
	}

	//int defs
	opt_int = (pst_conf_opt_int)conf_opt_int;
	while(opt_int->name){
		*(opt_int->value) = opt_int->default_value;
		_LOG_DBG("default config int: %s = %d", opt_int->name, *(opt_int->value));
		opt_int++;
	}

	//read config
	while(fgets(buf, CONF_STR_SIZE, fp)){

		key = trim_end(trim_start(buf));
		if(*key == '#' || *key == '/' || *key == ';' || *key == '\0')
			continue;

		if(!(val=strchr(key, '='))){
			_LOG_WRN("separator '=' not found in config string=%s", key);
			continue;
		}

		*val++ = 0;
		key = trim_end(key);

		//str vals
		opt_str = (pst_conf_opt_str)conf_opt_str;
		while(opt_str->name){
			if(!strcasecmp(key, opt_str->name)){
				*(opt_str->value) = strdup(trim_start(val));
				_LOG_DBG("got config str key=%s, val=%s", key, *(opt_str->value));
				break;
			}
			opt_str++;
		}

		//int vals if no str vals
		if(!opt_str->name){
			opt_int = (pst_conf_opt_int)conf_opt_int;
			while(opt_int->name){
				if(!strcasecmp(key, opt_int->name)){
					*(opt_int->value) = atoi(val);
					_LOG_DBG("got config int key=%s, val=%d", key, *(opt_int->value));
					break;
				}
				opt_int++;
			}
		}
	}

	fclose(fp);

	return 0;
}

int conf_init_nvr(const char *fname){
	st_conf_opt_str conf_opt_str[] = {
		{"user",							&g.conf.user,							NULL},

		{"alert_bind_host",					&g.conf.alert_bind_host,				DEF_ALERT_BIND_HOST},

		{"mysql_host",						&g.conf.mysql_host,						NULL},
		{"mysql_user",						&g.conf.mysql_user,						NULL},
		{"mysql_passwd",					&g.conf.mysql_passwd,					NULL},
		{"mysql_db",						&g.conf.mysql_db,						NULL},

		{"configs_path",					&g.conf.configs_path,					NULL},
		{"storage_path",					&g.conf.storage_path,					NULL},

		{NULL, NULL, NULL}
	};

	st_conf_opt_int conf_opt_int[] = {
		{"alert_bind_port",					&g.conf.alert_bind_port,				DEF_ALERT_BIND_PORT},

		{"mysql_port",						&g.conf.mysql_port,						DEF_MYSQL_PORT},
		{"mysql_connect_delay",				&g.conf.mysql_connect_delay,			DEF_MYSQL_CONN_DELAY},
		{"mysql_connect_tout",				&g.conf.mysql_connect_tout,				DEF_MYSQL_CONNECT_TOUT},
		{"mysql_read_tout",					&g.conf.mysql_read_tout,				DEF_MYSQL_READ_TOUT},
		{"mysql_write_tout",				&g.conf.mysql_write_tout,				DEF_MYSQL_WRITE_TOUT},
		{"mysql_retry_count",				&g.conf.mysql_retry_count,				DEF_MYSQL_RETRY_COUNT},
		{"mysql_retry_sleep",				&g.conf.mysql_retry_sleep,				DEF_MYSQL_RETRY_SLEEP},

		{"pk_read_tout",					&g.conf.pk_read_tout,					DEF_PK_READ_TOUT},
		{"pk_write_tout",					&g.conf.pk_write_tout,					DEF_PK_WRITE_TOUT},
		{"pk_max_data_size",				&g.conf.pk_max_data_size,				DEF_PK_MAX_DATA_SIZE},

		{"max_clients",						&g.conf.max_clients,					DEF_MAX_CLIENTS},
		{"max_clients_per_ip",				&g.conf.max_clients_per_ip,				DEF_MAX_CLIENTS_PER_IP},

		{"thread_stacksize",				&g.conf.thread_stacksize,				DEF_THREAD_STACKSIZE},

		{0, 0, 0}
	};

	return conf_init(fname, conf_opt_str, conf_opt_int);
}

int conf_init_stream(const char *fname, st_stream_conf *conf){
	st_conf_opt_str conf_opt_str[] = {
		{"save_path",		&conf->save_path,		NULL},
		{"rtsp_url",		&conf->rtsp_url,		NULL},

		{NULL, NULL, NULL}
	};

	st_conf_opt_int conf_opt_int[] = {
		{"min_run_time",	&conf->min_run_time,	DEF_STREAM_MIN_RUN_TIME},

		{0, 0, 0}
	};

	return conf_init(fname, conf_opt_str, conf_opt_int);
}
