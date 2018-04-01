#ifndef _DEFS_H
#define _DEFS_H

#define SZ(s) (sizeof(s)-1)
#define MAX_PATH					512

//_CONF_H
#define CONF_STR_SIZE				512

//_NVR_H
#define NVR_VERSION					"0.01"
#define SV_NAME_SIZE				255
#define SV_KICK_REASON_SIZE			255

#define FL_DAEMON					0x01
#define FL_CONFIG					0x02

#define DEF_MAX_PLAYERS				32
#define DEF_MAX_CLIENTS				1024
#define DEF_MAX_CLIENTS_PER_IP		64
#define DEF_THREAD_STACKSIZE		(128U * 1024)

#define DEF_ALERT_BIND_HOST			"0.0.0.0"
#define DEF_ALERT_BIND_PORT			15002

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

//_RCON_H
#define RCON_CMD_EXECCOMMAND		2
#define RCON_CMD_AUTH				3

#define RCON_RESPONSE_AUTH			2

#define RCON_CMD_SIZE				255
#define RCON_BUF_SIZE				8192

#define DEF_RCON_TOUT				15
#define DEF_RCON_INTERVAL			30
#define DEF_RCON_SLEEP_ERROR		90

//_SERVER_H
#define STATUS_PLAYERS_COUNT		"players :"
#define STATUS_STEAM00				"STEAM_0:0:"
#define STATUS_STEAM_ID_LAN			"STEAM_ID_LAN"
#define STATUS_STEAM_ID_PENDING		"STEAM_ID_PENDING"

//_PLAYERS_H
#define PL_STATUS_DISCONNECTED		0
#define PL_STATUS_PRE_DISCONNECT	1
#define PL_STATUS_CONNECTED			2
#define PL_STATUS_KICK				3
#define PL_STATUS_PENDING			4
#define PL_STATUS_LAN				5

#define DEF_TIME_OUT_VALIDATION		90
#define DEF_TIME_OUT_SCREEN_SHOT	1200

//_CLIENTS_H
#define INET_ADDR_STR_LEN			16

#define CL_LOG_STR					"[%s:%d]: "
#define CL_LOG(x)					x->ip_str, x->port

//_STREAMS_H
#define DEF_STREAM_MIN_RUN_TIME		60

#define ST_LOG_STR					"[%s]: "
#define ST_LOG(x)					x->ip_str

//_PROTOCOL_H
#define USE_CRYPT					0

#define SIGNED_GUID_LEN				32

#define DEBUG_ENCRYPTED_SV_PK		0
#define DEBUG_DECRYPTED_SV_PK		0
#define DEBUG_ENCRYPTED_CL_PK		0
#define DEBUG_DECRYPTED_CL_PK		0

#define NET_ALERT_HEADER_SIZE		20

#define DEF_PK_READ_TOUT			30
#define DEF_PK_WRITE_TOUT			30

#define DEF_PK_MAX_DATA_SIZE		10485760*20

#define DEF_CMD_INTERVAL			10
#define CMD_MASK					0x000000ff
#define CMD_NULL					0
#define CMD_PING					1
#define CMD_GSS						2
#define CMD_CLIENTCMD				3
#define CMD_SERVERCMD				4
#define CMD_SELFCRC32				5
#define CMD_SELFUPDATE				6
#define CMD_BREAK					254
#define CMD_EXIT					255

#endif
