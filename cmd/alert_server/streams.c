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

	_LOG_DBG("process stream for camera: %s", cl->camera_ip_str);

	//lock streams
	pthread_mutex_lock(&g.streams_mutex);

	//seek streams
	for(stream=g.streams; stream; stream=stream->next)
		if(stream->ip == cl->camera_ip){
			_LOG_DBG("found stream for %s, updating", cl->camera_ip_str);
			stream->updated = time(NULL);
			break;
		}

	//try to add stream
	if(!stream){
		if((stream=malloc(sizeof(st_stream)))){
			memset(stream, 0, sizeof(st_stream));
			stream->ip = cl->camera_ip;
			strcpy(stream->ip_str, cl->camera_ip_str);
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

#ifndef _LOG_DEBUG
	verbose = 0;
#endif

	_LOG_DBG(ST_LOG_STR"start stream thread", ST_LOG(st));

	dst = st->dst;
	len = sizeof(st->dst);
	snprintf(dst, len, g.conf.configs_path ? "%s/%s.conf" : "%s%s.conf", g.conf.configs_path ? g.conf.configs_path : "", st->ip_str);
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
	_LOG_INF(ST_LOG_STR"exit stream thread", ST_LOG(st));

	//remove from streams list
	pthread_mutex_lock(&g.streams_mutex);
	free_stream(&st);
	pthread_mutex_unlock(&g.streams_mutex);

	return NULL;
}
