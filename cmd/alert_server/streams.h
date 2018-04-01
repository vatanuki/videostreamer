#ifndef _STREAMS_H
#define _STREAMS_H

#include "structs.h"

void process_stream(st_client *cl);
void free_stream(st_stream **stream);
void unlink_stream(st_stream *stream);
void stream_insert_first(st_stream *stream);
void stream_remove(st_stream *stream);
void *stream_thread(void* av);
int stream_dbsync(st_stream *st);

#endif
