#ifndef _DEFS_H
#define _DEFS_H

#define SZ(s) (sizeof(s)-1)
#define MAX_PATH					512

//_CONF_H
#define CONF_STR_SIZE				512

//_NVR_H
#define NVR_VERSION					"0.02"

#define FL_DAEMON					0x01
#define FL_CONFIG					0x02
#define FL_NODB						0x04

#define DEF_THREAD_STACKSIZE		(128U * 1024)

#define DEF_ALERT_BIND_HOST			"0.0.0.0"
#define DEF_ALERT_BIND_PORT			15002

#define NET_ALERT_HEADER_SIZE		20

#define JSON_ALERT_MAX_TOKENS		64

//_DB_MYSQL_H
#define DEF_MYSQL_PORT				3306
#define DEF_MYSQL_CONN_DELAY		15
#define DEF_MYSQL_CONNECT_TOUT		30
#define DEF_MYSQL_READ_TOUT			30
#define DEF_MYSQL_WRITE_TOUT		30
#define DEF_MYSQL_RETRY_COUNT		30
#define DEF_MYSQL_RETRY_SLEEP		30

#define DB_MYSQL_CONNECTED			0x01
#define DB_MYSQL_PRE_QUERY_CHECK	0x02

#define DB_MYSQL_QTYPE_OTHER		0
#define DB_MYSQL_QTYPE_SELECT		1
#define DB_MYSQL_QTYPE_UPDATE		2
#define DB_MYSQL_QTYPE_INSERT		3

#define DB_MYSQL_QUERY_BUF_SIZE		2048

//_SOCK_H
#define S_RETRY						10
#define S_MAXPENDING				32

//_CLIENTS_H
#define INET_ADDR_STR_LEN			16

#define DEF_MAX_CLIENTS				1024
#define DEF_MAX_CLIENTS_PER_IP		64

#define DEF_PK_READ_TOUT			30
#define DEF_PK_WRITE_TOUT			30

#define CL_LOG_STR					"[%s:%d]: "
#define CL_LOG(x)					x->ip_str, x->port

//_STREAMS_H
#define DEF_STREAM_MIN_RUN_TIME		60

#define ST_LOG_STR					"[%s]: "
#define ST_LOG(x)					x->ip_str

#endif
