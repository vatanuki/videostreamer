#ifndef _TOOLS_H
#define _TOOLS_H

int dir_exists(const char *dname);
unsigned int file_size(const char *fname);
unsigned char *copy_xor_word(unsigned char *dst, const char *src, unsigned int len, unsigned short port);

#endif
