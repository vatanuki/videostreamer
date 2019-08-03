#include <stdio.h>
#include "defs.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <string.h>
#include <videostreamer.h>

#include "clients.h"
#include "tools.h"
#include "str.h"
#include "sock.h"
#include "db_mysql.h"
#include "nvr.h"
#include "log.h"
#include "structs.h"

st_g g;

void die(int nsig){
	_LOG_DBG("terminate, start cleanup");
	exit(0);
}

static void usage(char *self){
	fprintf(stderr, "Usage: %s -c config [-d daemon]\n", self);
	exit(-1);
}

static void sf(int nsig){
	_LOG_ERR("SEGMENTATION FAULT");
	exit(-1);
}

static void drop_privileges(){
	char *eos;
	struct passwd *pw;
	uid_t uid;

	if(g.conf.user){
		if(geteuid()){
			_LOG_WRN("geteuid: user=%s | %m", g.conf.user);
			return;
		}

		uid = strtol(g.conf.user, &eos, 10);
		if(!eos[0])
			//num
			pw = getpwuid(uid);
		else
			//name
			pw = getpwnam(g.conf.user);

		if(!pw){
			_LOG_ERR("getpwuid|getpwnam: user=%s | %m", g.conf.user);
			exit(-1);
		}

		if(setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1){
			_LOG_ERR("setgid|setuid: user=%s, uid=%d, gid=%d", g.conf.user, pw->pw_uid, pw->pw_gid);
			exit(-1);
		}

		//paranoia
		if(!setreuid(-1, 0)){
			_LOG_ERR("setreuid | %m");
			exit(-1);
		}

		_LOG_DBG("switched to user=%s, uid=%d, gid=%d", g.conf.user, pw->pw_uid, pw->pw_gid);
	}
}

int main(int ac, char **av){
	int oerrno, fd, osa_ok;
	unsigned int fl_start = 0;
	char ch;
	struct sigaction osa, sa;
	pid_t newgrp;

	//structure check
	if(sizeof(st_alert_header) != NET_ALERT_HEADER_SIZE){
		fprintf(stderr, "error in protocol structure: NET_ALERT_HEADER_SIZE[%d] != %d\n", NET_ALERT_HEADER_SIZE, (int)(sizeof(st_alert_header)));
		return -1;
	}

	setbuf(stdout, NULL);
	memset(&g, 0, sizeof(g));
	openlog(NULL, LOG_PID, LOG_USER);

	while((ch=getopt(ac, av, "c:d")) != EOF) switch(ch){
		case 'c':
			if(conf_init_nvr(optarg)){
				fprintf(stderr, "error conf_init\n");
				return -1;
			}
			fl_start|= FL_CONFIG;
			break;
		case 'd':
			fl_start|= FL_DAEMON;
			break;
	}

	if(!(fl_start & FL_CONFIG))
		usage(av[0]);

	if(!dir_exists(g.conf.storage_path)){
		fprintf(stderr, "please specify valid storage_path\n");
		return -1;
	}

	_LOG_INF("starting nvr: ver=%s", NVR_VERSION);

	//drop privileges
	drop_privileges();

	//daemon
	if(fl_start & FL_DAEMON){

		//a SIGHUP may be thrown when the parent exits below
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		osa_ok = sigaction(SIGHUP, &sa, &osa);

		switch(fork()){
			case -1:
				_LOG_ERR("fork | %m");
				return -1;
			case 0:
				break;
			default:
				_LOG_DBG("fork: exit parent");
				return 0;
		}

		newgrp = setsid();
		oerrno = errno;
		if(osa_ok != -1) sigaction(SIGHUP, &osa, NULL);
		if(newgrp == -1){
			errno = oerrno;
			_LOG_ERR("setsid | %m");
			return -1;
		}
		chdir("/");
		if((fd=open(_PATH_DEVNULL, O_RDWR, 0)) != -1){
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			if(fd > 2) close(fd);
		}
	}

	//signals
	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGTERM, die);
	signal(SIGSEGV, sf);
	signal(SIGPIPE, SIG_IGN);

	//init mutexes
	if(pthread_mutex_init(&g.clients_mutex, NULL)){
		_LOG_ERR("pthread_mutex_init | %m");
		return -1;
	}

	//threads attr
	if(pthread_attr_init(&g.thread_attr) || pthread_attr_setdetachstate(&g.thread_attr, PTHREAD_CREATE_DETACHED) || pthread_attr_setstacksize(&g.thread_attr, g.conf.thread_stacksize)){
		_LOG_ERR("pthread_attr_init || pthread_attr_setdetachstate || pthread_attr_setstacksize | %m");
		return -1;
	}

	//socket
	if((g.alert_sd=s_listen(g.conf.alert_bind_host, g.conf.alert_bind_port)) <= 0){
		_LOG_ERR("s_listen: sv_sd | %m");
		return -1;
	}

	//init db - clients threads
	init_db(1);

	g.db.query = "SET NAMES 'utf8'";
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	db_mysql_query(&g.db);

	g.db.query = "SET SESSION time_zone='Europe/Kiev'";
	g.db.query_type = DB_MYSQL_QTYPE_OTHER;
	db_mysql_query(&g.db);

	vs_setup();

	//clients loop
	accept_clients();

	return 0;
}
