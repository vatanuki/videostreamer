#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "str.h"

int buf_alloc(char **d, char *s, unsigned int *dl, unsigned int sl){
	if(*d){
		if(!(*d=realloc(*d, *dl+sl)))
			return -1;

	}else if(!(*d=malloc(*dl+sl)))
		return -1;

	memcpy(*d+*dl, s, sl);
	*dl+= sl;

	return 0;
}

void s_free(char **s){
	if(*s){
		free(*s);
		*s = NULL;
	}
}

char *trim_start(char *data){
	if(!data) return NULL;

	while(*data && isspace(*data)) ++data;

	return data;
}

char *trim_end(char *data){
	if(!data) return NULL;

	char* t = data+strlen(data);
	while(t > data && isspace(*(t-1))){
		t--;
		*t = 0;
	}

	return data;
}

char *straddslashes(char *dst, const char *src, char quot){
	char *seek;

	seek = dst;
	do
		if(*src == '\\' || *src == quot)
			*seek++ = '\\';
	while((*seek++ = *src++));

	return dst;
}

char *straddslashesdup(const char *src, char quot){
	char *dst;

	if(!(dst=malloc(strlen(src)*2+1)))
		return NULL;

	return straddslashes(dst, src, quot);
}

char *file2strchar(const char *fname, long *strlen){
	FILE *fp;
	char *fbuf;
	long flen;

	if(!fname || !*fname || !(fp=fopen(fname, "r")))
		return NULL;

	if(fseek(fp, 0, SEEK_END) == -1 || (flen=ftell(fp)) == -1 || fseek(fp, 0, SEEK_SET) == -1 || !(fbuf=(char*)malloc(flen+1))){
		fclose(fp);
		return NULL;
	}

	if(fread(fbuf, 1, flen, fp) != flen){
		fclose(fp);
		free(fbuf);
		return NULL;
	}

	fclose(fp);

	fbuf[flen] = 0;

	if(strlen)
		*strlen = flen;

	return fbuf;
}
