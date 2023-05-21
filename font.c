/* See LICENSE file for copyright and license details. */

#include "font.h"
#include "verbose.h"
#include "dmenu.h"


#define UTF_INVALID     0xFFFD
#define UTF_SIZ         4


static const byte d_utf_byte [UTF_SIZ + 1] = { 0x80,     0,    0xC0,  0xE0,   0xF0 };
static const byte d_utf_mask [UTF_SIZ + 1] = { 0xC0,     0x80, 0xE0,  0xF0,   0xF8 };
static const long d_utf_min  [UTF_SIZ + 1] = { 0,        0,    0x80,  0x800,  0x10000 };
static const long d_utf_max  [UTF_SIZ + 1] = { 0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF };

const char fnt_dots [4] = "...";


long
utf8_decode_byte (byte val, uint *ret)
{
    uint i;
    const byte *pm, *pb;
    byte m;
    
    for ( i = 0, pm = d_utf_mask, pb = d_utf_byte; i <= UTF_SIZ; i++, pm++, pb++ ) {
        m = *pm;
        if ( ( val & m ) == *pb ) {
            *ret = i;
            return val & ~m;
        }
    }
    *ret = 0;
    return 0;
}

long
utf8_between (long u, uint i)
{
    if ( !BETWEEN (u, d_utf_min[i], d_utf_max[i]) ||
          BETWEEN (u, 0xD800, 0xDFFF) )
        return UTF_INVALID;
    
    return u;
}

uint
utf8_validate (long *u, uint i)
{
    const long *pm;
    long val;
    
    val = *u = utf8_between (*u, i);
    for ( i = 1, pm = d_utf_max + 1; val > *pm; i++, pm++ )
        ; /* NOP */
    
    return i;
}

uint
utf8_decode (const char *c, long *u)
{
    long decoded;
    uint len, type, i, clen;
    
    decoded = utf8_decode_byte ((byte) *c, &len);
    if ( !BETWEEN (len, 1, UTF_SIZ) ) {
        *u = UTF_INVALID;
        return 1;
    }
    for ( i = 1, clen = MIN (len, UTF_SIZ); i < clen; i++ ) {
        decoded = (decoded << 6) | utf8_decode_byte ((byte) *++c, &type);
        if ( type != 0 ) {
            *u = UTF_INVALID;
            return i;
        }
    }
    *u = utf8_between (decoded, len);
    return len;
}

int
fnt_open_name (Fnt *fnt, const char *name)
{
    XftFont *font;
    FcPattern *pattern;

    /* Using the pattern found at font->xfont->pattern does not yield
     * the same substitution results as using the pattern returned by
     * FcNameParse; using the latter results in the desired fallback
     * behaviour whereas the former just results in missing-character
     * rectangles being drawn, at least with some fonts. */
    font = XftFontOpenName (d_drw.dpy, d_drw.screen, name);
    if ( font == NULL ) {
        error ("cannot load font from name: '%s'", name);
        return False;
    }

    /* pattern */
    pattern = FcNameParse ((FcChar8 *) name);
    if ( pattern == NULL ) {
        error ("cannot parse font name to pattern: '%s'", name);
        XftFontClose (d_drw.dpy, font);
        return False;
    }

    /* set structure members */
    fnt->font = font;
    fnt->pattern = pattern;
    fnt->h = font->ascent + font->descent;
    fnt->dots = fnt_get_text_width (font, fnt_dots, countof (fnt_dots));
    
    return True;
}
 
int
fnt_open_pattern (Fnt *fnt, FcPattern *pattern)
{
    XftFont *font;

    font = XftFontOpenPattern (d_drw.dpy, pattern);
    if ( font == NULL ) {
        error ("cannot load font from pattern");
        return False;
    }

    /* set structure members */
    fnt->font = font;
    fnt->pattern = pattern;
    fnt->h = font->ascent + font->descent;
    fnt->dots = fnt_get_text_width (font, fnt_dots, countof (fnt_dots));
    
    return True;
}

void 
fnt_free (Fnt *fnt)
{
    FcPatternDestroy (fnt->pattern);
    XftFontClose (d_drw.dpy, fnt->font);
}

void
fnt_free_v (Fnt *fnt, uint size)
{
    while ( size-- != 0 )
        fnt_free (fnt++);
}

int
fnt_get_text_width (XftFont *font, const char *text, int len)
{
    XGlyphInfo ext;

    if ( len == -1 )
        len = strlen (text);

    XftTextExtentsUtf8 (d_drw.dpy, font, (XftChar8 *)text, len, &ext);
    return ext.xOff;
}

Fnt *
fnt_char_exists (Fnt *fonts, uint size, long utf8codepoint)
{
    while ( size-- != 0 ) {
        if ( XftCharExists (d_drw.dpy, fonts->font, utf8codepoint) )
            return fonts;
        fonts++;
    }
    return NULL;
}

FcPattern *
fnt_create_pattern_utf8 (Fnt *fnt, long utf8codepoint)
{
    FcCharSet *fccharset;
    FcPattern *match;
    FcPattern *fcpattern;
    XftResult result;

    /* create charset */
    fccharset = FcCharSetCreate ();
    FcCharSetAddChar (fccharset, utf8codepoint);

    /* create pattern */
    fcpattern = FcPatternDuplicate (fnt->pattern);
    FcPatternAddCharSet (fcpattern, FC_CHARSET, fccharset);
    FcPatternAddBool (fcpattern, FC_SCALABLE, FcTrue);
    FcPatternAddBool (fcpattern, FC_COLOR, FcFalse);
    FcConfigSubstitute (NULL, fcpattern, FcMatchPattern);
    FcDefaultSubstitute (fcpattern);

    match = XftFontMatch (d_drw.dpy, d_drw.screen, fcpattern, &result);

    /* destroy pattern */
    FcPatternDestroy (fcpattern);

    /* destroy charset */
    FcCharSetDestroy (fccharset);

    return match;
}

const char *
fnt_parse_text_utf8 (const char *text, XftFont *font, uint *utf8len, long *utf8codepoint)
{
    uint char_len, text_len = 0;
    long code_point;

    while ( *text != '\0' ) {
        /* decode characters */
        char_len = utf8_decode (text, &code_point);

        /* check first font in the list */
        if ( !XftCharExists (d_drw.dpy, font, code_point) ) {
            *utf8len = text_len;
            *utf8codepoint = code_point;
            return text;
        }
        text_len += char_len;
        text += char_len;
    }
    /* $utf8codepoint is not used because the whole text is parsed */
    *utf8len = text_len;
    return NULL;
}
