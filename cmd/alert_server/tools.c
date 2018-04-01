#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "str.h"
#include "tools.h"
#include "structs.h"

extern st_g g;

int dir_exists(const char *dname){
	DIR *dir;

	if(!dname || !(dir=opendir(dname)))
		return 0;

	closedir(dir);
	return 1;
}

unsigned int file_size(const char *fname){
	struct stat buf;

	if(!fname || stat(fname, &buf))
		return 0;

	return buf.st_size;
}

unsigned char *copy_xor_word(unsigned char *dst, const char *src, unsigned int len, unsigned short port){
	unsigned int idx;

	memcpy(dst, src, len);
	len/= sizeof(unsigned short);
	for(idx=0; idx < len; idx++)
		((unsigned short *)dst)[idx]^= (unsigned short)(port*port*(idx+1));

	return dst;
}
