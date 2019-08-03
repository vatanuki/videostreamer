#ifndef _CLIENTS_H
#define _CLIENTS_H

#include "structs.h"

void accept_clients(void);
void free_client(st_client **client);
void unlink_client(st_client *client);
void client_insert_first(st_client *client);
void client_remove(st_client *client);
void *client_thread(void* av);
int client_dbsync(st_client *cl);
int client_alert_data(st_client *cl);

#endif
