/* See LICENSE file for copyright and license details. */

#ifndef _DRW_H_
#define _DRW_H_

#include "font.h"
#include "menuitem.h"
#include "thunk.h"


#define drw_get_text_width(s)  (drw_text (0, 0, ~0, (s), ColorMax))
#define drw_fonts              ((Fnt* ) d_drw.fonts.items)

#define drw_padding_y  2


typedef enum {
    ColorWindow,    /* window background color */
    ColorItem,      /* normal item text color */
    ColorDisabled,  /* disabled item text color */
    ColorText,      /* selected item text color */
    ColorSelected,  /* selected item background color */
    ColorMax
} Color;

typedef struct {
    Display *dpy;
    Drawable drawable;
    GC gc;
    Thunk fonts;
    Window root;
    uint w, h;
    Color fg, bg;
    int screen;
} Drw;

typedef enum {
    DrawRender = (1 << 0),
    DrawInvert = (1 << 1),
    DrawFilled = (1 << 2)
} DrawFlags;


extern uint drw_padding_x;
extern uint drw_bh, drw_mw, drw_mh;


/* Drawable abstraction */
int drw_create (void);
int drw_init (void);
void drw_free (void);
void drw_resize (uint w, uint h);

/* Colorscheme abstraction */
int drw_colors_create (XftColor *colors, const char **names, uint size);
void drw_colors_destroy (XftColor* colors, uint size);
void drw_colors_init (XftColor *colors, uint size);

/* Cursor abstraction */
Cursor drw_cur_create (int shape);
void drw_cur_destroy (Cursor cursor);

/* Drawing functions */
void drw_rect (uint x, uint y, uint w, uint h, Color color);
void drw_fill (uint x, uint y, uint w, uint h, Color color);
int drw_text (uint x, uint y, uint w, const char *text, Color color);

/* Map functions */
void drw_map (uint x, uint y, uint w, uint h);

int drw_font_add_v (const char **fonts, uint size);
Fnt * drw_fnt_open_name (const char *name);
Fnt * drw_fnt_open_pattern (FcPattern *pattern);
Fnt * drw_fnt_create_utf8 (long utf8codepoint);

int drw_draw_item (MenuItem *item, int y);
int drw_draw_menu (void);


#endif  /* _DRW_H_ */
