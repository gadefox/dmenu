/* See LICENSE file for copyright and license details. */

#ifndef _UTIL_H_
#define _UTIL_H_

#include "typedef.h"


extern int   (*x_strncmp) (const char *, const char *, size_t);
extern char *(*x_strstr)  (const char *, const char *);


void usage (void);
void version (void);

void *x_malloc (uint size);
void *x_realloc (void *ptr, uint size);

const char * get_prog_name (const char *name);

char * ci_strstr (const char *s, const char *sub);
void case_insensitive (void);

int intersect (int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
int intersect_area (int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);


#endif  /* _UTIL_H_ */
