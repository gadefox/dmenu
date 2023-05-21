/* See LICENSE file for copyright and license details. */

#ifndef _MENUITEM_H_
#define _MENUITEM_H_

#include <X11/Xlib.h>
#include <stdio.h>
#include "thunk.h"


#define mi_get_middle_x(p)  ((p)->x + ((p)->w >> 1))
#define mi_width(w)         ((w) + (drw_padding_x << 1))
#define mi_is_focused()     (mi_sel != NULL && mi_sel->flags & MenuItemHilighted)


typedef enum {
    MouseWindow,
    MouseLeftArrow,
    MouseRightArrow,
    MouseInput,
    MouseMenuItem
} MouseStatus;

typedef enum {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
} Corner;

typedef enum {
    MenuItemDisabled  = (1 << 0),
    MenuItemHilighted = (1 << 1)
} MenuItemFlags;

typedef struct _MenuItem {
    struct _MenuItem *next;
    struct _MenuItem *prev;
    char *text;
    uint x;
    uint w;
    uint row;
    MenuItemFlags flags;
} MenuItem;

typedef struct {
    MenuItem *items;
    MenuItem *tail;
} MenuItems;

extern Thunk mi_items;
extern MenuItems mi_matches;
extern MenuItem *mi_topleft, *mi_sel, *mi_next_page;


int mi_init (void);
void mi_free (void);
int mi_read_file (FILE *file);
int mi_read (const char *name);
int mi_read_file_or_stdin (void);

void mi_append_item (MenuItems *list, MenuItem *item);
void mi_append_List (MenuItems *list, MenuItems *append);
int mi_match (void);
void mi_select (MenuItem *sel, int hilight);

MenuItem * mi_get_next_row (MenuItem* item, uint rows);
MenuItem * mi_get_prev_row (MenuItem* item, uint rows);
MenuItem * mi_get_next_at_pos (MenuItem* item, uint x);
MenuItem * mi_get_prev_at_pos (MenuItem* item, uint x);

uint mi_layout_page (void);
void mi_ensure_visible (MenuItem *item, MenuItem *sel, Corner corner);
void mi_layout_bottom_right_rows (MenuItem* item, uint rows, int mw);
void mi_layout_bottom_left (MenuItem *item);
void mi_layout_top_left (MenuItem *item);
void mi_layout_bottom_right (MenuItem *item);
void mi_layout_top_right (MenuItem *item);
int mi_layout_last_page (void);

int mi_handle_click (uint x, uint y);
MouseStatus mi_get_mouse_status (uint x, uint y, MenuItem **ret);

void mi_update_geometry (void);
int mi_key_press (KeySym ksym, uint state, char *buf, uint len);


#endif  /* _MENUITEM_H_ */
