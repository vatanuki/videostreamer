#ifndef _SOCK_H
#define _SOCK_H

#include <sys/socket.h>

#include "defs.h"

int s_connect(char *ip, int port, int t_out, char *b_ip);
int s_read(int sd, char *buf, int size, int t_out);
int s_read_all(int sd, char *data, unsigned int data_size, int t_out);
int s_read_line(int sd, char *buf, int size, int t_out);
int s_write(int sd, char *buf, int size, int t_out);
int s_write_all(int sd, char *data, unsigned int data_size, int t_out);
void s_close(int *sock);
int s_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int s_listen(char *b_host, int port);

#endif
