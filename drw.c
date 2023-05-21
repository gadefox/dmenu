/* See LICENSE file for copyright and license details. */

#include "drw.h"
#include "verbose.h"
#include "dmenu.h"
#include "args.h"
#include "input.h"


#define drw_buffer_begin  ((char *) drw_buffer.items)


static Thunk drw_buffer = { NULL };

uint drw_padding_x;
uint drw_bh, drw_mw, drw_mh;


int
drw_init (void)
{
    return thunk_create (&drw_buffer, 256, sizeof (char)) != NULL;
}

int
drw_create (void)
{
    if ( thunk_create (&d_drw.fonts, 0, sizeof (Fnt) ) == NULL )
        return False;

    d_drw.gc = XCreateGC (d_drw.dpy, d_drw.root, 0, NULL);
    XSetLineAttributes (d_drw.dpy, d_drw.gc, 1, LineSolid, CapButt, JoinMiter);
    return True;
}

void
drw_resize (uint w, uint h)
{
    d_drw.w = w;
    d_drw.h = h;

    if ( d_drw.drawable != None )
        XFreePixmap (d_drw.dpy, d_drw.drawable);

    d_drw.drawable = XCreatePixmap (d_drw.dpy, d_drw.root, w, h,
           DefaultDepth (d_drw.dpy, d_drw.screen));
}

void
drw_free (void)
{
    /* buffer */
    thunk_free (&drw_buffer);
        
    /* fonts */
    fnt_free_v (drw_fonts, d_drw.fonts.nelements);
    thunk_free (&d_drw.fonts);

    if ( d_drw.drawable != None )
        XFreePixmap (d_drw.dpy, d_drw.drawable);

    if ( d_drw.gc != NULL )
        XFreeGC (d_drw.dpy, d_drw.gc);

    if ( d_win != None )
        XDestroyWindow (d_drw.dpy, d_win);
        
    XUngrabKey (d_drw.dpy, AnyKey, AnyModifier, d_drw.root);
 
    if ( d_drw.dpy != NULL ) {
        XSync (d_drw.dpy, False);
        XCloseDisplay (d_drw.dpy);
    }
}

int
drw_colors_create (XftColor *colors, const char **names, uint size)
{
    const char *name;

    while ( size-- != 0 ) {
        name = *names++;
        if ( !XftColorAllocName (d_drw.dpy,
               DefaultVisual (d_drw.dpy, d_drw.screen),
               DefaultColormap (d_drw.dpy, d_drw.screen),
               name, colors) ) {
            error ("cannot allocate color '%s'", name);
            return False;
        }
        colors++;
    }
    return True;
}

void
drw_colors_destroy (XftColor* colors, uint size)
{
    while ( size-- != 0 ) {
        XftColorFree (d_drw.dpy,
                  DefaultVisual (d_drw.dpy, d_drw.screen),
                  DefaultColormap (d_drw.dpy, d_drw.screen),
                  colors);
        colors++;
    }
}

void
drw_colors_init (XftColor *colors, uint size)
{
    while ( size-- != 0 ) {
        colors->pixel = 0;
        colors++;
    }
}

void
drw_fill (uint x, uint y, uint w, uint h, Color color)
{
    XSetForeground (d_drw.dpy, d_drw.gc, d_color [color].pixel);
    XFillRectangle (d_drw.dpy, d_drw.drawable, d_drw.gc, x, y, w, h);
}

void
drw_rect (uint x, uint y, uint w, uint h, Color color)
{
    XSetForeground (d_drw.dpy, d_drw.gc, d_color [color].pixel);
    XDrawRectangle (d_drw.dpy, d_drw.drawable, d_drw.gc, x, y, w - 1, h - 1);
}

/* $text can't be empty: len > 0 */
static int
drw_text_cut (XftFont *font, uint w, uint dots, const char *text, uint *len)
{
    uint low, high, mid, ew;

    /* check the buffer size and reallocate */
    high = *len;
    if ( high <= 3 || w <= dots ) {
        strncpy (drw_buffer_begin, fnt_dots, countof (fnt_dots));
        *len = countof (fnt_dots);
        return dots;
    }

    /* check buffer size and reallocate if necessary */
    if ( high > drw_buffer.alloc_size &&
        thunk_double_size (&drw_buffer, high) == NULL )
        return -1;

    w -= dots;
    high -= 3;  /* 3 dots */
    memcpy (drw_buffer.items, text, high * sizeof (char));

    /* binary search */
    low = 1;
    while ( low <= high ) {
        /* pivot */
        mid = (low + high) >> 1;
        ew = fnt_get_text_width (font, drw_buffer_begin, mid);
        if ( ew < w )
            low = mid + 1;
        else if ( ew > w )
            high = mid - 1;
        else {
            low = mid;
            break;
        }
    }
    
    strncpy (drw_buffer_begin + low, fnt_dots, countof (fnt_dots));
    *len = low + countof (fnt_dots);

    ew = fnt_get_text_width (font, drw_buffer_begin, low);
    return ew + dots;
}

int
drw_text (uint x, uint y, uint w, const char *text, Color color)
{
    Fnt *cur_fnt;
    XftFont *cur_font;
    long utf8codepoint = 0;
    const char *utf8text;
    uint ew, utf8len, fnt_y;
    XftDraw *d = NULL;

    if ( color != ColorMax )
        d = XftDrawCreate (d_drw.dpy, d_drw.drawable,
                           DefaultVisual (d_drw.dpy, d_drw.screen),
                           DefaultColormap (d_drw.dpy, d_drw.screen));

    for ( cur_fnt = drw_fonts; ; ) {
        cur_font = cur_fnt->font;
        utf8text = text;

        /* decode text */
        text = fnt_parse_text_utf8 (text, cur_font, &utf8len, &utf8codepoint);

        /* empty text? */
        if ( utf8len != 0 ) {
            /* check the width */
            ew = fnt_get_text_width (cur_font, utf8text, utf8len);
            if ( ew > w ) {
                ew = drw_text_cut (cur_font, w, cur_fnt->dots, utf8text, &utf8len);
                if ( ew == -1 )
                    return -1;

                utf8text = drw_buffer_begin;
            }
            if ( color != ColorMax ) {
                fnt_y = y + ((drw_bh - cur_fnt->h) >> 1) + cur_font->ascent;
                XftDrawStringUtf8 (d, &d_color [color], cur_font, x, fnt_y, (XftChar8 *) utf8text, utf8len);
            }
            x += ew;
            w -= ew;

            if ( utf8text == drw_buffer_begin )
                /* we don't have the space for drawing remaining text */
                break;
        }

        /* fnt_parse_text_utf8 returns null when the whole text has been parsed */
        if ( text == NULL )
            break;

        /* note we checked current font already (see fnt_parse_text_utf8) */
        cur_fnt = fnt_char_exists ((Fnt *) d_drw.fonts.items, d_drw.fonts.nelements, utf8codepoint);
        if ( cur_fnt != NULL )
            continue;
     
        /* create new font which can draw utf8 character */
        cur_fnt = drw_fnt_create_utf8 (utf8codepoint);
        if ( cur_fnt != NULL )
            continue;

        /* regardless of whether or not a fallback font is found, the character must be drawn. */
        utf8codepoint = -1;
        cur_fnt = (Fnt *)d_drw.fonts.items;
   }

    if ( d != NULL )
        XftDrawDestroy (d);

    return x;
}

void
drw_map (uint x, uint y, uint w, uint h)
{
    XCopyArea (d_drw.dpy, d_drw.drawable, d_win, d_drw.gc, x, y, w, h, x, y);
    XSync (d_drw.dpy, False);
}

Cursor
drw_cursor_create (int shape)
{
    return XCreateFontCursor (d_drw.dpy, shape);
}

void
drw_cursor_destroy (Cursor cursor)
{
    if ( cursor != None )
         XFreeCursor (d_drw.dpy, cursor);
}

int
drw_font_add_v (const char **fonts, uint size)
{
    while ( size-- != 0 ) {
        if ( drw_fnt_open_name (*fonts++) == NULL )
            return False;
    }
    return True;
}

Fnt *
drw_fnt_open_name (const char *name)
{
    Fnt *fnt;
    FcBool iscol;

    /* add new fnt item to the list */
    fnt = (Fnt *) thunk_alloc_next (&d_drw.fonts);
    if ( fnt == NULL )
        return NULL;

    if ( !fnt_open_name (fnt, name) ) {
        /* remove font from the array */
        d_drw.fonts.nelements--;
        return NULL;
    }

    /* Do not allow using color fonts. This is a workaround for a
     * BadLength error from Xft with color glyphs. Modelled on the Xterm
     * workaround. See https://bugzilla.redhat.com/show_bug.cgi?id=1498269
     * https://lists.suckless.org/dev/1701/30932.html
     * https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=916349 and lots
     * more all over the internet. */
    if ( FcPatternGetBool (fnt->pattern, FC_COLOR, 0, &iscol) == FcResultMatch && iscol ) {
        fnt_free (fnt);

        /* remove font from the array */
        d_drw.fonts.nelements--;
        return NULL;
    }

    return fnt;
}

Fnt *
drw_fnt_open_pattern (FcPattern *pattern)
{
    Fnt *fnt;
    FcBool iscol;

    /* add new font container */
    fnt = (Fnt *) thunk_alloc_next (&d_drw.fonts);
    if ( fnt == NULL )
        return NULL;

    if ( !fnt_open_pattern (fnt, pattern) )
        goto quit;

    /* Do not allow using color fonts. This is a workaround for a
     * BadLength error from Xft with color glyphs. Modelled on the Xterm
     * workaround. See https://bugzilla.redhat.com/show_bug.cgi?id=1498269
     * https://lists.suckless.org/dev/1701/30932.html
     * https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=916349 and lots
     * more all over the internet. */
    if ( FcPatternGetBool (fnt->pattern, FC_COLOR, 0, &iscol) == FcResultMatch && iscol ) {
        /* destroy only xfont, we're not responsible for the pattern */
        XftFontClose (d_drw.dpy, fnt->font);
        goto quit;
    }

    return fnt;

quit:

    /* remove font from the array */
    d_drw.fonts.nelements--;
    return NULL;
}

Fnt *
drw_fnt_create_utf8 (long utf8codepoint)
{
    FcPattern *pattern;
    Fnt *fnt;

    pattern = fnt_create_pattern_utf8 ((Fnt *) d_drw.fonts.items, utf8codepoint);
    if ( pattern == NULL )
        return NULL;

    fnt = drw_fnt_open_pattern (pattern);
    if ( fnt == NULL ) {
        FcPatternDestroy (pattern);
        return NULL;
    }

    if ( !XftCharExists (d_drw.dpy, fnt->font, utf8codepoint) ) {
        /* remove font from the array */
        d_drw.fonts.nelements--;
        /* destroy the font */
        fnt_free (fnt);
        return NULL;
    }

    return fnt;
}

int
drw_draw_item (MenuItem *item, int y)
{
    Color color;

    /* hilighted? */    
    if ( item->flags & MenuItemHilighted ) {
        drw_fill (item->x, y, item->w, drw_bh, ColorSelected);
        color = ColorText;
    } else {
        /* selected? */
        if ( item == mi_sel )
            drw_rect (item->x, y, item->w, drw_bh, ColorSelected);
        color = ColorItem;
    }
    /* disabled? */
    if ( item->flags & MenuItemDisabled )
        color = ColorDisabled;
   
    return drw_text (item->x + drw_padding_x, y, item->w - (drw_padding_x << 1), item->text, color);
}

static void
drw_draw_arrowleft (uint x)
{
    XPoint pts [4];
    XPoint *p;
    uint tmp;
    
    /* init */
    tmp = drw_bh >> 1;

    /* point0 */
    p = pts;
    p->x = x;
    p->y = tmp;

    /* point1 */
    p++;
    p->x = tmp;
    p->y = tmp;

    /* point2 */
    p++;
    p->x = 0;
    p->y = -drw_bh;

    /* color */
    XSetForeground (d_drw.dpy, d_drw.gc, d_color [ColorSelected].pixel);

    /* draw triangle */
    if ( mi_is_focused () )
        XFillPolygon (d_drw.dpy, d_drw.drawable, d_drw.gc, pts, 3, Convex, CoordModePrevious);
    else {
        /* point3 = point0 */
        p++;
        p->x = -tmp;
        p->y = tmp;

        XDrawLines (d_drw.dpy, d_drw.drawable, d_drw.gc, pts, 4, CoordModePrevious);
    }
}

static void
drw_draw_arrowright (uint y)
{
    XPoint pts [4];
    XPoint *p;
    uint tmp;
    
    /* init */
    tmp = drw_bh >> 1;

    /* point0 */
    p = pts;
    p->x = drw_mw - tmp;
    p->y = y;

    /* point1 */
    p++;
    p->x = tmp;
    p->y = tmp;

    /* point2 */
    p++;
    p->x = -tmp;
    p->y = tmp;

    /* color */
    XSetForeground (d_drw.dpy, d_drw.gc, d_color [ColorSelected].pixel);

    /* draw triangle */
    if ( mi_is_focused () )
        XFillPolygon (d_drw.dpy, d_drw.drawable, d_drw.gc, pts, 3, Convex, CoordModePrevious);
    else {
        /* point3 = point0 */
        p++;
        p->x = 0;
        p->y = -drw_bh;

        XDrawLines (d_drw.dpy, d_drw.drawable, d_drw.gc, pts, 4, CoordModePrevious);
    }
}

int
drw_draw_menu (void)
{
    MenuItem *item;
    uint y, row;
    char c;

    /* background */
    drw_fill (0, 0, drw_mw, drw_mh, ColorWindow);

    /* prompt */
    drw_text (drw_padding_x, 0, a_prompt_w, a_prompt, ColorItem);

    /* input */
    row = input.w - drw_padding_x;
    drw_text (a_prompt_w, 0, row, input_begin, ColorItem);

    /* cursor */
    if ( !mi_is_focused () ) {
        c = *input.cursor;
        *input.cursor = '\0';

        /* $y is used as a temp variable */
        y = drw_get_text_width (input_begin);
        *input.cursor = c;
        
        if ( y < row )
            drw_fill (a_prompt_w + y, 3, 1, drw_bh - 5, ColorItem);
    }

    /* left arrow */
    if ( mi_topleft != mi_matches.items )
        drw_draw_arrowleft (a_prompt_w + input.w);

    /* items */
    y = 0;
    row = 0;
    for ( item = mi_topleft; item != NULL; item = item->next ) {
        /* check item row */
        if ( item->row != row ) {
            if ( ++row == a_rows )
                break;
            y += drw_bh;
        }
        /* draw item */
        if ( drw_draw_item (item, y) == -1 )
            return False;
    }
    /* right arrow */
    if ( mi_next_page != NULL )
        drw_draw_arrowright (y);
    
    drw_map (0, 0, drw_mw, drw_mh);
    return True;
}
