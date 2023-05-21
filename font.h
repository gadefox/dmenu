/* See LICENSE file for copyright and license details. */

#ifndef _FONT_H_
#define _FONT_H_

#include <X11/Xft/Xft.h>
#include "typedef.h"


typedef struct {
    XftFont *font;
    FcPattern *pattern;
    uint h;
    uint dots;
} Fnt;


extern const char fnt_dots [4];


int fnt_open_name (Fnt *fnt, const char *name);
int fnt_open_pattern (Fnt *fnt, FcPattern *pattern);

void fnt_free (Fnt *fnt);
void fnt_free_v (Fnt *fnt, uint size);

int fnt_get_text_width (XftFont *font, const char *text, int len);

const char * fnt_parse_text_utf8 (const char *text, XftFont *font, uint *utf8len, long *utf8codepoint);
Fnt * fnt_char_exists (Fnt *fonts, uint size, long utf8codepoint);
FcPattern * fnt_create_pattern_utf8 (Fnt *fnt, long utf8codepoint);

/* utf8 */
long utf8_decode_byte (byte val, uint *ret);
uint utf8_validate (long *u, uint i);
long utf8_between (long u, uint i);
uint utf8_decode (const char *c, long *u);


#endif  /* _FONT_H_ */
