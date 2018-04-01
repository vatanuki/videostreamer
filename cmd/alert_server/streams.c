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
#include "streams.h"
#include "log.h"
#include "structs.h"

extern st_g g;

void process_stream(st_client *cl){
	st_stream *stream;

	_LOG_DBG("process stream for client: %s", cl->ip_str);

	//lock streams
	pthread_mutex_lock(&g.streams_mutex);

	//seek streams
	for(stream=g.streams; stream; stream=stream->next)
		if(stream->ip == cl->ip){
			_LOG_DBG("found stream, updating");
			stream->updated = time(NULL);
			break;
		}

	//try to add stream
	if(!stream){
		if((stream=malloc(sizeof(st_stream)))){
			memset(stream, 0, sizeof(st_stream));
			stream->ip = cl->ip;
			strcpy(stream->ip_str, cl->ip_str);
			stream->updated = time(NULL);

			//add stream to streams list
			stream_insert_first(stream);

			//start stream thread
			if(pthread_create(&stream->thread_id, &g.thread_attr, stream_thread, stream)){
				_LOG_ERR("pthread_create: stream_thread | %m");
				free_stream(&stream);
			}

		}else{
			_LOG_ERR("malloc: stream | %m");
		}
	}

	//unlock streams
	pthread_mutex_unlock(&g.streams_mutex);
}

void free_stream(st_stream **stream){
	if(*stream){
		stream_remove(*stream);

		if((*stream)->conf.save_path)
			free((*stream)->conf.save_path);

		if((*stream)->conf.rtsp_url)
			free((*stream)->conf.rtsp_url);

		if((*stream)->input)
			vs_destroy_input((*stream)->input);

		if((*stream)->output)
			vs_destroy_output((*stream)->output);

		free(*stream);
		*stream = NULL;
	}
}

void stream_insert_first(st_stream *stream){
	if(g.streams)
		g.streams->prev = stream;

	stream->next = g.streams;
	g.streams = stream;
}

void stream_remove(st_stream *stream){
	if(stream->prev)
		((st_stream*)stream->prev)->next = stream->next;
	else
		g.streams = stream->next;

	if(stream->next)
		((st_stream*)stream->next)->prev = stream->prev;
}

void *stream_thread(void* av){
	char *dst;
	unsigned int len, cnt, verbose = 1;
	st_stream *st = av;
	time_t t;
	struct tm *lt;
	AVPacket pkt;

	_LOG_DBG(ST_LOG_STR"start stream thread", ST_LOG(st));

	dst = st->dst;
	len = sizeof(st->dst);
	snprintf(dst, len, "%s%s.conf", g.conf.configs_path ? g.conf.configs_path : "", st->ip_str);
	conf_init_stream(dst, &st->conf);

	if(!st->conf.save_path || !st->conf.rtsp_url){
		_LOG_ERR(ST_LOG_STR"missing stream config (%s): save_path=%s, rtsp_url=%s", ST_LOG(st), dst, st->conf.save_path, st->conf.rtsp_url);
		goto st_bye;
	}

	cnt = snprintf(dst, len, "file:%s/%s/", g.conf.storage_path, st->conf.save_path);
	if(!dir_exists(st->dst+5) && mkdir(st->dst+5, 0755)){
		_LOG_ERR(ST_LOG_STR"stream_thread: mkdir(%s) | %m", ST_LOG(st), st->dst);
		goto st_bye;
	}

	dst+= cnt;
	len-= cnt;
	t = time(NULL);
	if(!(lt=localtime(&t)) || !(cnt=strftime(dst, len, "%Y-%m-%d/", lt))){
		_LOG_ERR(ST_LOG_STR"stream_thread: localtime or strftime | %m", ST_LOG(st));
		goto st_bye;
	}

	if(!dir_exists(st->dst+5) && mkdir(st->dst+5, 0755)){
		_LOG_ERR(ST_LOG_STR"stream_thread: mkdir(%s) | %m", ST_LOG(st), st->dst);
		goto st_bye;
	}

	dst+= cnt;
	len-= cnt;
	if(!(cnt=strftime(dst, len, "%s.mp4", lt))){
		_LOG_ERR(ST_LOG_STR"stream_thread: strftime", ST_LOG(st));
		goto st_bye;
	}

	_LOG_INF(ST_LOG_STR"stream dst=%s, rtsp_url=%s", ST_LOG(st), st->dst, st->conf.rtsp_url);

	if(!(st->input = vs_open_input("rtsp", st->conf.rtsp_url, verbose))){
		_LOG_ERR(ST_LOG_STR"stream_thread: vs_open_input %s", ST_LOG(st), st->conf.rtsp_url);
		goto st_bye;
	}

	if(!(st->output = vs_open_output("mp4", st->dst, st->input, verbose))){
		_LOG_ERR(ST_LOG_STR"stream_thread: vs_open_output %s", ST_LOG(st), st->dst);
		goto st_bye;
	}

	do{
		//grab timer
		pthread_mutex_lock(&g.streams_mutex);
		t = time(NULL) - st->updated;
		pthread_mutex_unlock(&g.streams_mutex);

		//_LOG_DBG(ST_LOG_STR"stream thread idle time: %lu", ST_LOG(st), t);

		memset(&pkt, 0, sizeof(pkt));
		if((cnt = vs_read_packet(st->input, &pkt, verbose)) == -1){
			_LOG_ERR(ST_LOG_STR"stream_thread: vs_read_packet", ST_LOG(st));
			break;
		}

		if(cnt == 0)
			continue;

		if((cnt = vs_write_packet(st->input, st->output, &pkt, verbose)) == -1){
			_LOG_ERR(ST_LOG_STR"stream_thread: vs_write_packet", ST_LOG(st));
			av_packet_unref(&pkt);
			break;
		}
		av_packet_unref(&pkt);

	}while(t < st->conf.min_run_time);

st_bye:
	_LOG_DBG(ST_LOG_STR"exit stream thread", ST_LOG(st));

	//remove from streams list
	pthread_mutex_lock(&g.streams_mutex);
	free_stream(&st);
	pthread_mutex_unlock(&g.streams_mutex);

	return NULL;
}
/*
int stream_dbsync(st_stream *cl){
	int ret;
	char query[DB_MYSQL_QUERY_BUF_SIZE], *raw;

	_LOG_DBG(CL_LOG_STR"add alert to db", CL_LOG(cl));


	_LOG_DBG(CL_LOG_STR"alert: (0x%08x 0x%08x 0x%08x 0x%08x): (%u) %s", CL_LOG(cl),
		cl->alert_header.ui1, cl->alert_header.ui2, cl->alert_header.ui3, cl->alert_header.ui4,
		cl->alert_header.data_size, (char*)cl->pk_data);


	snprintf(query, sizeof(query), "INSERT INTO alerts SET ip=\"%s\", raw=\"(0x%08x 0x%08x 0x%08x 0x%08x): %s\"", cl->ip_str, cl->alert_header.ui1, cl->alert_header.ui2, cl->alert_header.ui3, cl->alert_header.ui4, (raw = straddslashesdup((char*)cl->pk_data, '"')));
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	ret = db_mysql_query(&g.db);
	pthread_mutex_unlock(&g.db.mysql_mutex);

	if(raw)
		free(raw);

	return ret ? -1 : 0;
}
*/
/*
int stream_dbsync(st_stream *cl){
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int ret, affected_rows;
	unsigned long *lengths;
	char query[DB_MYSQL_QUERY_BUF_SIZE];

	//chain
	if(cl->chain){
		cl->sv_header.cmd = cl->chain;
		cl->chain = 0;
		return 2;
	}

	//default command
	cl->sv_header.cmd = CMD_PING;

	//do we need to update db
	if(!cl->hw_id || cl->user_id <= 0 || !cl->steam_id[0]){
		_LOG_DBG(CL_LOG_STR"skip: no ident information", CL_LOG(cl));
		return 1;
	}

	//fetch command
	snprintf(query, sizeof(query), "SELECT cmd,params FROM streams WHERE hw_id=%u AND cmd AND user_id=%d AND steam_id=\"%s\" AND ip=\"%s\"", cl->hw_id, cl->user_id, cl->steam_id, cl->ip_str);
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_SELECT;
	if(!(ret=db_mysql_query(&g.db)) && g.db.num_rows > 0){
		res = g.db.res;
		g.db.res = NULL;
	}
	pthread_mutex_unlock(&g.db.mysql_mutex);

	if(ret)
		return -1;

	if(res){
		if((row=mysql_fetch_row(res))){
			_LOG_DBG(CL_LOG_STR"fetch command: cmd=%s, params=%s", CL_LOG(cl), row[0], row[1]);

			cl->sv_header.cmd = atoi(row[0]);
			lengths = mysql_fetch_lengths(res);
			if((cl->sv_header.data_size=lengths[1])){
				s_free((char**)&cl->pk_data);
				cl->sv_header.data_size++;
				if((cl->pk_data=malloc(cl->sv_header.data_size))){
					memcpy(cl->pk_data, row[1], lengths[1]);
					((char*)cl->pk_data)[lengths[1]] = 0;
				}else{
//					_LOG_ERR(CL_LOG_STR"malloc: pk_data %li bytes | %m", CL_LOG(cl), g.loader_size);
				}
			}
		}

		mysql_free_result(res);
	}

	//gss
//	if(g.conf.gss_path && cl->sv_header.cmd == CMD_PING && cl->gss_last + g.conf.gss_interval + ((int)((float)(g.conf.gss_random)*rand()/(RAND_MAX+1.0))) < time(NULL))
//		cl->sv_header.cmd = CMD_GSS;

	//update stream
	snprintf(query, sizeof(query), "UPDATE streams SET user_id=%d, steam_id=\"%s\", ip=\"%s\", cmd=0, updated=now() WHERE hw_id=%u", cl->user_id, cl->steam_id, cl->ip_str, cl->hw_id);
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_UPDATE;
	if(!(ret=db_mysql_query(&g.db)))
		affected_rows = g.db.affected_rows;
	pthread_mutex_unlock(&g.db.mysql_mutex);

	if(ret)
		return -1;

	if(affected_rows > 0)
		return 0;

	_LOG_DBG(CL_LOG_STR"new db stream", CL_LOG(cl));

	snprintf(query, sizeof(query), "INSERT INTO streams SET hw_id=%u, user_id=%d, steam_id=\"%s\", ip=\"%s\", cmd=0, updated=now()", cl->hw_id, cl->user_id, cl->steam_id, cl->ip_str);
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	ret = db_mysql_query(&g.db);
	pthread_mutex_unlock(&g.db.mysql_mutex);

	return ret ? -1 : 0;
}

void cmd_answer_gss(st_stream *cl){
	FILE *fp;
	char fname[MAX_PATH], gss_path[MAX_PATH], query[DB_MYSQL_QUERY_BUF_SIZE];

	cl->gss_last = time(NULL);

	if(cl->cl_header.result == FRS_OK){

		snprintf(fname, sizeof(fname), "%s/%u/%lu.jpg", g.conf.gss_path, cl->hw_id, cl->gss_last);
		if(!(fp=fopen(fname, "wb")) && errno == ENOENT){

			snprintf(gss_path, sizeof(gss_path), "%s/%u", g.conf.gss_path, cl->hw_id);
			if(!mkdir(gss_path, 0755))
				fp = fopen(fname, "wb");
		}

		if(fp){
			if(fwrite(cl->pk_data, 1, cl->cl_header.data_size, fp) != cl->cl_header.data_size){
				_LOG_ERR(CL_LOG_STR"fwrite: pk_data %u bytes to %s | %m", CL_LOG(cl), cl->cl_header.data_size, fname);
				cl->cl_header.result = FRS_GSS_FWRITE_ERR;
			}
			fclose(fp);
		}else{
			_LOG_ERR(CL_LOG_STR"fopen: %s | %m", CL_LOG(cl), fname);
			cl->cl_header.result = FRS_GSS_FOPEN_ERR;
		}
	}

	snprintf(query, sizeof(query), "INSERT INTO gss SET hw_id=%u, timestamp=%lu, result=%u", cl->hw_id, cl->gss_last, cl->cl_header.result);
	pthread_mutex_lock(&g.db.mysql_mutex);
	g.db.query = query;
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	db_mysql_query(&g.db);
	pthread_mutex_unlock(&g.db.mysql_mutex);
}

void cmd_answer_selfcrc32(st_stream *cl){
	if(!cl->cl_header.result){
		_LOG_DBG(CL_LOG_STR"enter loop mode: result=0x%08x", CL_LOG(cl), cl->cl_header.result);
		return;
	}

	cl->chain = CMD_BREAK;
	if(!g.loader_size || !g.loader_data || !g.loader_crc32){
		_LOG_WRN(CL_LOG_STR"auto update disabled", CL_LOG(cl));
		return;
	}

	if(cl->cl_header.result == g.loader_crc32){
		_LOG_DBG(CL_LOG_STR"no need to update: crc32=0x%08x", CL_LOG(cl), cl->cl_header.result);
		return;
	}

	s_free((char**)&cl->pk_data);
	if(!(cl->pk_data=malloc(g.loader_size))){
		_LOG_ERR(CL_LOG_STR"malloc: pk_data %li bytes | %m", CL_LOG(cl), g.loader_size);
		return;
	}

	memcpy(cl->pk_data, g.loader_data, g.loader_size);
	cl->sv_header.data_size = g.loader_size;
	cl->chain = CMD_SELFUPDATE;

	_LOG_DBG(CL_LOG_STR"update: size=%li, crc32=0x%08x", CL_LOG(cl), g.loader_size, g.loader_crc32);
}

void cmd_answer_selfupdate(st_stream *cl){
	if(cl->cl_header.result != 1){
		_LOG_ERR(CL_LOG_STR"CMD_SELFUPDATE: result=%d", CL_LOG(cl), cl->cl_header.result);
		cl->chain = CMD_BREAK;
	}else
		cl->chain = CMD_EXIT;
}
*/