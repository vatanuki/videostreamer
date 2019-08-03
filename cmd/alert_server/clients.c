#include <stdio.h>
#include "defs.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>

#include "sock.h"
#include "str.h"
#include "tools.h"
#include "db_mysql.h"
#include "clients.h"
#include "log.h"
#include "structs.h"
#include "streams.h"
#include "jsmn.h"

extern st_g g;

void accept_clients(void){
	int cl_sd, cl_port;
	unsigned int cnt, cnt_ip;
	char cl_ip[INET_ADDR_STR_LEN];
	struct sockaddr_in cl_addr;
	st_client *client;

	_LOG_INF("starting accept clients");

	for(;;){
		cnt = sizeof(cl_addr);
		if((cl_sd=accept(g.alert_sd, (struct sockaddr *)&cl_addr, &cnt)) <= 0){
			_LOG_ERR("accept | %m");
			continue;
		}

		if(!inet_ntop(AF_INET, &cl_addr.sin_addr, cl_ip, sizeof(cl_ip))){
			_LOG_ERR("inet_ntop | %m");
			close(cl_sd);
			continue;
		}
		cl_port = ntohs(cl_addr.sin_port);

		_LOG_DBG("incoming: %s:%d", cl_ip, cl_port);

		//lock clients
		pthread_mutex_lock(&g.clients_mutex);

		//count connected clients
		if(g.conf.max_clients || g.conf.max_clients_per_ip){
			for(cnt=0,cnt_ip=0,client=g.clients;
				client && (!g.conf.max_clients || cnt < g.conf.max_clients) && (!g.conf.max_clients_per_ip || cnt_ip < g.conf.max_clients_per_ip);
				cnt++,client=client->next)
					if(client->ip == cl_addr.sin_addr.s_addr)
						cnt_ip++;
			_LOG_DBG("count connected: cnt=%d cnt_ip=%d", cnt, cnt_ip);
		}

		//try to add client
		if((client=malloc(sizeof(st_client)))){
			memset(client, 0, sizeof(st_client));
			client->sd = cl_sd;
			client->ip = cl_addr.sin_addr.s_addr;
			strcpy(client->ip_str, cl_ip);
			strcpy(client->camera_ip_str, cl_ip);
			client->port = cl_port;

			//add client to clients list
			client_insert_first(client);

			//start client thread
			if(pthread_create(&client->thread_id, &g.thread_attr, client_thread, client)){
				_LOG_ERR("pthread_create: client_thread | %m");
				free_client(&client);
			}

		}else{
			_LOG_ERR("malloc: client | %m");
		}

		//unlock clients
		pthread_mutex_unlock(&g.clients_mutex);
	}
}

void free_client(st_client **client){
	if(*client){
		client_remove(*client);

		if((*client)->pk_data)
			free((*client)->pk_data);

		free(*client);
		*client = NULL;
	}
}

void client_insert_first(st_client *client){
	if(g.clients)
		g.clients->prev = client;

	client->next = g.clients;
	g.clients = client;
}

void client_remove(st_client *client){
	if(client->prev)
		((st_client*)client->prev)->next = client->next;
	else
		g.clients = client->next;

	if(client->next)
		((st_client*)client->next)->prev = client->prev;
}

void *client_thread(void* av){
	st_client *cl = av;
	_LOG_DBG("connected: %s:%d", cl->ip_str, cl->port);

	if(s_read_all(cl->sd, (char*)&cl->alert_header, sizeof(cl->alert_header), g.conf.pk_read_tout)){
		_LOG_ERR(CL_LOG_STR"s_read_all: alert_header", CL_LOG(cl));
		goto cl_bye;
	}

	if(!(cl->pk_data=malloc(cl->alert_header.data_size+1))){
		_LOG_ERR(CL_LOG_STR"malloc: alert_data %u bytes | %m", CL_LOG(cl), cl->alert_header.data_size);
		goto cl_bye;
	}

	if(s_read_all(cl->sd, (char*)cl->pk_data, cl->alert_header.data_size, g.conf.pk_read_tout)){
		_LOG_ERR(CL_LOG_STR"s_read_all: alert_data %u bytes | %m", CL_LOG(cl), cl->alert_header.data_size);
		goto cl_bye;
	}

	((char*)cl->pk_data)[cl->alert_header.data_size] = 0;
	trim_end((char*)cl->pk_data);

	_LOG_INF(CL_LOG_STR"alert: (0x%08x 0x%08x 0x%08x 0x%08x): (%u) %s", CL_LOG(cl),
		cl->alert_header.ui1, cl->alert_header.ui2, cl->alert_header.ui3, cl->alert_header.ui4,
		cl->alert_header.data_size, (char*)cl->pk_data);

	//1: Type=Alarm, 2: Event=MotionDetect
	if(client_alert_data(cl) == 2)
		process_stream(cl);

	client_dbsync(cl);

cl_bye:
	s_close(&cl->sd);
	_LOG_DBG(CL_LOG_STR"disconnected", CL_LOG(cl));

	//remove from clients list
	pthread_mutex_lock(&g.clients_mutex);
	free_client(&cl);
	pthread_mutex_unlock(&g.clients_mutex);

	return NULL;
}

int client_dbsync(st_client *cl){
	int ret;
	char query[DB_MYSQL_QUERY_BUF_SIZE], *raw;

	_LOG_DBG(CL_LOG_STR"add alert to db", CL_LOG(cl));

	snprintf(query, sizeof(query), "INSERT INTO alerts SET ip=\"%s\", raw=\"%s\"", cl->ip_str, (raw = straddslashesdup((char*)cl->pk_data, '"')));
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	ret = db_mysql_query(&g.db);
	pthread_mutex_unlock(&g.db.mysql_mutex);

	if(raw)
		free(raw);

	return ret ? -1 : 0;
}

int client_alert_data(st_client *cl){
	int r = 0, i, cnt;
	char n, v, *name, *value, *data = (char*)cl->pk_data;
	struct in_addr addr;
	jsmn_parser p;
	jsmntok_t t[JSON_ALERT_MAX_TOKENS];

	jsmn_init(&p);
	if((cnt=jsmn_parse(&p, data, cl->alert_header.data_size, t, JSON_ALERT_MAX_TOKENS)) <= 0 || t[0].type != JSMN_OBJECT || !t[0].size){
		_LOG_ERR(CL_LOG_STR"jsmn_parse: %s", CL_LOG(cl), data);
		return -1;
	}

	for(i = 1; i < cnt; i++){
		if(t[i].type != JSMN_STRING || t[i].size != 1 || t[i+1].size)
			continue;

		n = data[t[i].end];
		v = data[t[i+1].end];
		data[t[i].end] = data[t[i+1].end] = '\0';

		name = data + t[i].start;
		value = data + t[i+1].start;

		_LOG_DBG(CL_LOG_STR"json: %s = %s", CL_LOG(cl), name, value);

		if(!strcmp(name, "Address")){
			if((addr.s_addr = strtol(value, NULL, 16)))
				strcpy(cl->camera_ip_str, inet_ntoa(addr));

		}else if(!strcmp(name, "Type")){
			if(!strcmp(value, "Alarm") && !r)
				r = 1;

		}else if(!strcmp(name, "Event")){
			if(!strcmp(value, "MotionDetect"))
				r = 2;
		}

		data[t[i].end] = n;
		data[t[i+1].end] = v;
		i++;
	}

	return r;
}
