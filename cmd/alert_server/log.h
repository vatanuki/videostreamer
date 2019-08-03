#ifndef _LOG_H
#define _LOG_H

#include <syslog.h>
#include <mysql/mysql.h>

/*
'__FILE__' %s
'__LINE__' %d
'__DATE__' %s -> "Mmm dd yyyy"
'__TIME__' %s -> "hh:mm:ss"
'__TIMESTAMP__' %s -> "Mmm dd yyyy hh:mm:ss"
'__FUNCTION__' %s
*/

#ifdef LOG_TO_FILE

#ifdef LOG_TO_FILE_EMAIL
	//file
	#define _LOG_ERR(b,args...) filelogem("[-] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_WRN(b,args...) filelogem("[W] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_DBG(b,args...) filelogem("[D] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_INF(b,args...) filelogem("[I] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);

	#define _LOG_DB_ERR(a,b,args...) filelogem("[-] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));
	#define _LOG_DB_WRN(a,b,args...) filelogem("[W] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));

	void filelogem(const char* format, ...);
#else
	//file
	#define _LOG_ERR(b,args...) filelogth("[-] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_WRN(b,args...) filelogth("[W] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_DBG(b,args...) filelogth("[D] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_INF(b,args...) filelogth("[I] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);

	#define _LOG_DB_ERR(a,b,args...) filelogth("[-] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));
	#define _LOG_DB_WRN(a,b,args...) filelogth("[W] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));

	void filelogth(const char* format, ...);
#endif

#else

#ifdef LOG_TO_SYSLOG
	//syslog user
	#define _LOG_ERR(b,args...) syslog(LOG_ERR,"[-] %s:%d: "b,__FUNCTION__,__LINE__,##args);
	#define _LOG_WRN(b,args...) syslog(LOG_WARNING,"[W] %s:%d: "b,__FUNCTION__,__LINE__,##args);
	#define _LOG_DBG(b,args...) syslog(LOG_DEBUG,"[D] %s:%d: "b,__FUNCTION__,__LINE__,##args);
	#define _LOG_INF(b,args...) syslog(LOG_INFO,"[I] %s:%d: "b,__FUNCTION__,__LINE__,##args);

	#define _LOG_DB_ERR(a,b,args...) syslog(LOG_ERR,"[-] %s:%d: "b" | %s",__FUNCTION__,__LINE__,##args,mysql_error(a));
	#define _LOG_DB_WRN(a,b,args...) syslog(LOG_WARNING,"[W] %s:%d: "b" | %s",__FUNCTION__,__LINE__,##args,mysql_error(a));
#else
	//stdout
	#define _LOG_ERR(b,args...) printf("[-] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#define _LOG_WRN(b,args...) printf("[W] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#ifdef _LOG_DEBUG
		#define _LOG_DBG(b,args...) printf("[D] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);
	#else
		#define _LOG_DBG(b,args...)
	#endif
	#define _LOG_INF(b,args...) printf("[I] %s:%d: "b"\n",__FUNCTION__,__LINE__,##args);

	#define _LOG_DB_ERR(a,b,args...) printf("[-] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));
	#define _LOG_DB_WRN(a,b,args...) printf("[W] %s:%d: "b" | %s\n",__FUNCTION__,__LINE__,##args,mysql_error(a));
#endif

#endif

#endif
