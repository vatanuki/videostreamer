#include <memory.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "sock.h"
#include "log.h"

int s_connect(char *ip, int port, int t_out, char *b_ip){
	int sd, flags, error;
	struct sockaddr_in s_addr, b_addr;
	struct timeval tv = {t_out, 0};
	fd_set rfds, wfds;
	socklen_t error_len;

	//inet
	if((sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		_LOG_ERR("socket | %m");
		return -1;
	}

	//FD_SETSIZE
	if(FD_SETSIZE <= sd){
		_LOG_ERR("connect: descriptor %d does not fit FD_SETSIZE %d", sd, FD_SETSIZE);
		s_close(&sd);
		return -1;
	}

	memset((char *)&(s_addr), 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(port);

	//resolv ip
	if(!inet_aton(ip, (struct in_addr*)&s_addr.sin_addr.s_addr)){
		_LOG_ERR("inet_aton: ip=%s", ip);
		s_close(&sd);
		return -1;
	}

	//bind
	if(b_ip){
		memset((char *)&(b_addr), 0, sizeof(b_addr));
		b_addr.sin_family = AF_INET;
		b_addr.sin_port = 0;

		//resolv ip
		if(!inet_aton(b_ip, (struct in_addr*)&b_addr.sin_addr.s_addr)){
			_LOG_ERR("inet_aton: b_ip=%s", b_ip);
			s_close(&sd);
			return -1;
		}

		//bind
		if(bind(sd, (struct sockaddr *)&b_addr, sizeof(b_addr))){
			_LOG_ERR("bind: b_ip=%s | %m", b_ip);
			s_close(&sd);
			return -1;
		}

		_LOG_DBG("bind: b_ip=%s", b_ip);
	}

	//get flags
	if((flags=fcntl(sd, F_GETFL, 0)) < 0){
		_LOG_ERR("F_GETFL | %m");
		s_close(&sd);
		return -1;
	}

	//remove block
	if(fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0){
		_LOG_ERR("F_SETFL | %m");
		s_close(&sd);
		return -1;
	}

	//connect
	if(connect(sd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0){

		if(errno != EINPROGRESS){
			_LOG_ERR("connect: errno != EINPROGRESS | %m");
			s_close(&sd);
			return -1;
		}

		//read || write wait
		FD_ZERO(&rfds);
		FD_SET(sd, &rfds);
		FD_ZERO(&wfds);
		FD_SET(sd, &wfds);

		switch(s_select(sd+1, &rfds, &wfds, NULL, &tv)){
		case -1:
			_LOG_ERR("s_select");
			s_close(&sd);
			return -1;
		case 0:
			_LOG_ERR("timeout");
			s_close(&sd);
			errno = ETIMEDOUT;
			return -2;
		}

		if(FD_ISSET(sd, &rfds) || FD_ISSET(sd, &wfds)){

			//continue check
			error = 0;
			error_len = sizeof(error);
			if(getsockopt(sd, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0){
				_LOG_ERR("getsockopt | %m");
				s_close(&sd);
				return (-1);
			}

			if(error){
				errno = error;
				_LOG_ERR("socket: error | %m");
				s_close(&sd);
				return -1;
			}

		}else{
			_LOG_ERR("FD_ISSET | %m");
			s_close(&sd);
			return -1;
		}
	}

	//restore flags
	if(fcntl(sd, F_SETFL, flags) < 0){
		_LOG_ERR("F_SETFL | %m");
		s_close(&sd);
		return -1;
	}

	_LOG_DBG("connected: ip=%s", ip);
	return sd;
}

int s_read(int sd, char *buf, int size, int t_out){
	int rez;
	fd_set rfds;
	struct timeval tv = {t_out, 0};

	if(sd <= 0 || sd >= FD_SETSIZE || !buf){
		_LOG_ERR("input");
		return -1;
	}

	//prepare
	FD_ZERO(&rfds);
	FD_SET(sd, &rfds);

	if((rez=s_select(sd+1, &rfds, NULL, NULL, &tv)) == -1){
		_LOG_ERR("s_select");
		return -1;
	}

	if(!rez){
		_LOG_ERR("timeout");
		return -2;
	}

	if(FD_ISSET(sd, &rfds)){
		if((rez=read(sd, buf, size)) < 0){
			_LOG_ERR("read | %m");
		}
		return rez;
	}else{
		_LOG_ERR("not FD_ISSET | %m");
		return -1;
	}
}

int s_read_all(int sd, char *data, unsigned int data_size, int t_out){
	int cnt;
	unsigned int read_cnt = 0;

	do
		if((cnt=s_read(sd, &data[read_cnt], data_size-read_cnt, t_out)) > 0)
			read_cnt+= cnt;
	while(cnt > 0 && read_cnt != data_size);

	if(cnt < 0){
		_LOG_ERR("s_read: read_cnt=%u, data_size=%u | %m", read_cnt, data_size);
		return cnt;
	}

	if(read_cnt != data_size){
		_LOG_ERR("s_read: read_cnt=%d, data_size=%d", read_cnt, data_size);
		return -1;
	}

	return 0;
}

int s_read_line(int sd, char *buf, int size, int t_out){
	int idx, rez;

	if(sd <= 0 || sd >= FD_SETSIZE || !buf){
		_LOG_ERR("input");
		return -1;
	}

	idx = 0;
	if(size > 0){
		while(idx < size-1){
			if((rez=s_read(sd, &buf[idx], 1, t_out)) < 0){
				_LOG_ERR("s_read");
				return rez;
			}
			if(!rez || buf[idx] == '\n') break;
			idx++;
		}
		buf[idx] = 0;
	}

	return idx;
}

int s_write(int sd, char *buf, int size, int t_out){
	int rez;
	fd_set wfds;
	struct timeval tv = {t_out, 0};

	if(sd <= 0 || sd >= FD_SETSIZE || !buf){
		_LOG_ERR("input");
		return -1;
	}

	//prepare
	FD_ZERO(&wfds);
	FD_SET(sd, &wfds);

	if((rez=s_select(sd+1, NULL, &wfds, NULL, &tv)) == -1){
		_LOG_ERR("s_select");
		return -1;
	}

	if(!rez){
		_LOG_ERR("timeout");
		return -2;
	}

	if(FD_ISSET(sd, &wfds)){
		if((rez=write(sd, buf, size)) < 0){
			_LOG_ERR("write | %m");
		}
		return rez;
	}else{
		_LOG_ERR("not FD_ISSET | %m");
		return -1;
	}
}

int s_write_all(int sd, char *data, unsigned int data_size, int t_out){
	int cnt;
	unsigned int write_cnt = 0;

	do
		if((cnt=s_write(sd, &data[write_cnt], data_size-write_cnt, t_out)) > 0)
			write_cnt+= cnt;
	while(cnt > 0 && write_cnt != data_size);

	if(cnt < 0){
		_LOG_ERR("s_write: write_cnt=%d, data_size=%d | %m", write_cnt, data_size);
		return cnt;
	}

	if(write_cnt != data_size){
		_LOG_ERR("s_write: write_cnt=%d, data_size=%d", write_cnt, data_size);
		return -1;
	}

	return 0;
}

void s_close(int *sock){
	int error;

	if(*sock > 0){
		error = errno;
		close(*sock);
		errno = error;
		*sock = 0;
	}
}

int s_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
	int rez, retry = S_RETRY;

	do rez = select(n, readfds, writefds, exceptfds, timeout);
	while(rez == -1 && errno == EINTR && retry--);

	if(rez == -1){
		_LOG_ERR("select | %m");
	}

	return rez;
}

int s_listen(char *b_ip, int port){
	int sd, on = 1;
	struct sockaddr_in b_addr;

	//inet
	if((sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		_LOG_ERR("socket | %m");
		return -1;
	}

	//FD_SETSIZE
	if(FD_SETSIZE <= sd){
		_LOG_ERR("connect: descriptor %d does not fit FD_SETSIZE %d", sd, FD_SETSIZE);
		s_close(&sd);
		return -1;
	}

	//SO_REUSEADDR
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))){
		_LOG_ERR("setsockopt | %m");
		s_close(&sd);
		return -1;
	}

	//fill struct
	memset((char *)&(b_addr), 0, sizeof(b_addr));
	b_addr.sin_family = AF_INET;
	b_addr.sin_port = htons(port);
	if(b_ip && !inet_aton(b_ip, (struct in_addr*)&b_addr.sin_addr.s_addr)){
		_LOG_ERR("inet_aton: b_ip=%s", b_ip);
		s_close(&sd);
		return -1;
	}

	//bind
	if(bind(sd, (struct sockaddr *)&b_addr, sizeof(b_addr))){
		_LOG_ERR("bind: %s:%d | %m", b_ip ? b_ip : "0.0.0.0", port);
		s_close(&sd);
		return -1;
	}

	//listen
	if(listen(sd, S_MAXPENDING)){
		_LOG_ERR("listen: S_MAXPENDING=%d, %s:%d | %m", S_MAXPENDING, b_ip ? b_ip : "0.0.0.0", port);
		s_close(&sd);
		return -1;
	}

	_LOG_DBG("listen: S_MAXPENDING=%d, %s:%d", S_MAXPENDING, b_ip ? b_ip : "0.0.0.0", port);
	return sd;
}
