#ifndef _STR_H
#define _STR_H

int buf_alloc(char **d, char *s, unsigned int *dl, unsigned int sl);
void s_free(char **s);
char *trim_start(char *data);
char *trim_end(char *data);
char *straddslashes(char *dst, const char *src, char quot);
char *straddslashesdup(const char *src, char quot);
char *file2strchar(const char *fname, long *strlen);

#endif

