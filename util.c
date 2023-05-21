/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "verbose.h"


int   (*x_strncmp) (const char *, const char *, size_t) = strncmp;
char *(*x_strstr)  (const char *, const char *)         = strstr;


const char *
get_prog_name (const char *name)
{
    char *p;

    p = strrchr (name, '/');
    if ( p != NULL )
        return p + 1;

    return name;
}

void
usage (void)
{
    help ("usage: [-bfiv] [-l lines] [-p prompt] [-fn font] [-m monitor]\n"
           "      [-nb color] [-nf color] [-sb color] [-sf color] [-ab color]\n"
           "      [-af color] [-w windowid]");
}

void
version (void)
{
    verbose_s (VERSION);
    verbose_newline ();
}

void *
x_malloc (uint size)
{
    void *p = malloc (size);
    if ( p == NULL ) {
        error (msg_out_of_memory);
        return NULL;
    }
    return p;
}

void *
x_realloc (void *p, uint size)
{
    void *ret;

    ret = realloc (p, size);
    if ( ret == NULL ) {
        error (msg_out_of_memory);
        return NULL;
    }
    return ret;
}

char *
ci_strstr (const char *s, const char *sub)
{
    uint len;

    for ( len = strlen (sub); *s != '\0'; s++ ) {
        if ( strncasecmp (s, sub, len) == 0 )
            return (char *) s;
    }
    return NULL;
}

void
case_insensitive (void)
{
    x_strncmp = strncasecmp;
    x_strstr = ci_strstr;
}

int
intersect (int x1, int y1, int w1, int h1,
           int x2, int y2, int w2, int h2)
{
    int r1 = x1 + w1;
    int r2 = x2 + w2;
    int x = MIN (r1, r2) - MAX (x1, x2);

    int b1 = y1 + h1;
    int b2 = y2 + h2;
    int y = MIN (b1, b2) - MAX (y1, y2);
    
    return MAX (0, x) && MAX (0, y);
}

int
intersect_area (int x1, int y1, int w1, int h1,
                int x2, int y2, int w2, int h2)
{
    int r1 = x1 + w1;
    int r2 = x2 + w2;
    int x = MIN (r1, r2) - MAX (x1, x2);

    int b1 = y1 + h1;
    int b2 = y2 + h2;
    int y = MIN (b1, b2) - MAX (y1, y2);

    return MAX (0, x) * MAX (0, y);
}
