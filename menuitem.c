/* See LICENSE file for copyright and license details. */

#include <ctype.h>
#include "menuitem.h"
#include "strutil.h"
#include "util.h"
#include "verbose.h"
#include "args.h"
#include "input.h"
#include "dmenu.h"


#define menuitems_init(o)  { (o).items = NULL; (o).tail = NULL; }


Thunk mi_items = { NULL };

MenuItems mi_matches;
MenuItem *mi_topleft, *mi_next_page, *mi_sel;


/* returns the first item in the next row */
MenuItem *
mi_get_next_row (MenuItem* item, uint rows)
{
    uint row;
   
    row = item->row + rows;
    for ( item = item->next; item != NULL; item = item->next ) {
        if ( item->row == row )
            return item;
    }
    return NULL;
}

/* returns the last item in the previous row */
MenuItem *
mi_get_prev_row (MenuItem* item, uint rows)
{
    uint row;

    row = item->row - rows;
    for ( item = item->prev; item != NULL; item = item->prev ) {
        if ( item->row == row )
            return item;
    }
    return NULL;
}

/* the item must be the first in the row */
MenuItem *
mi_get_next_at_pos (MenuItem* item, uint x)
{
    MenuItem *prev;
    uint row;

    row = item->row + 1;
    for ( ;; ) {
        prev = item;
        item = item->next;

        if ( item == NULL ||      /* the previous item was the last in the list */
             item->row == row ||  /* this item belongs to the next row */
             item->x > x )       /* x is between prev->x and prev->x + prev->w */
            return prev;
    }
    /* NOP */
}

/* the item must be the last in the row */
MenuItem *
mi_get_prev_at_pos (MenuItem* item, uint x)
{
    MenuItem *prev;
    uint row;
        
    /* check the fist item in the row */
    if ( item->x <= x )
        return item;

    row = item->row - 1;
    for ( ;; ) {
        prev = item;
        item = item->prev;

        if ( item == NULL ||
             item->row == row )  /* this item belongs to the previous row */
            return prev;

        if ( item->x <= x )
            return item;
    }
    return NULL;
}

int
mi_init (void)
{
    return thunk_create (&mi_items, 128, sizeof (MenuItem)) != NULL;
}

void
mi_free (void)
{
    thunk_free (&mi_items);
}

void
mi_append_item (MenuItems *list, MenuItem *item)
{
    MenuItem *tail;

    item->next = NULL;

    /* tail */
    tail = list->tail;
    list->tail = item;

    if ( tail != NULL ) {
        /* connect tail w/ items */
        tail->next = item;
        item->prev = tail;
    }
    else {
        /* list is empty */
        list->items = item;
        item->prev = NULL;
    }
}

void
mi_append_list (MenuItems *list, MenuItems *append)
{
    MenuItem *tail;
    MenuItem *items;
  
    /* tail */
    tail = list->tail;
    list->tail = append->tail;  /* append->tail cannot be null */

    items = append->items;  /* append->items cannot be null */
    if ( tail == NULL )
        /* list is empty */
        list->items = items;
    else {
        /* connect tail w/ items */
        tail->next = items;
        items->prev = tail;
    }
}

void
mi_select (MenuItem *sel, int hilight)
{
    if ( mi_sel != NULL )
        mi_sel->flags &= ~MenuItemHilighted;
    mi_sel = sel;
    if ( hilight )
        mi_sel->flags |= MenuItemHilighted;
}

int
mi_match (void)
{
    char *buf, *first;
    uint idx, len, textlen;
    MenuItems prefix;
    MenuItems substr;
    MenuItem *mi;

    /* create new buffer and update tokens */
    buf = input_update_tokens ();
    if ( buf == NULL )
        return -1;

    menuitems_init (prefix);
    menuitems_init (substr);
    menuitems_init (mi_matches);
 
    if ( input_tok.nelements == 0 )
        len = 0;
    else {
        first = *input_tok_v;
        len = strlen (first);
    } 
    
    textlen = input.t.nelements;

    for ( idx = mi_items.nelements, mi = (MenuItem *) mi_items.items; idx != 0; idx--, mi++ ) {
        if ( input_tok.nelements == 0 ) {
            /* no tokens so add the item to the list */
            mi_append_item (&mi_matches, mi);
            continue;
        }
        /* disabled? */
        if ( mi->flags & MenuItemDisabled )
            continue;
        /* filter */
        if ( !input_check_tokens (mi->text) )
            continue;

        /* exact matches go first, then prefixes, then substrings */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        if ( x_strncmp (mi->text, input_begin, textlen) == 0 )
            mi_append_item (&mi_matches, mi);
        else if ( x_strncmp (first, mi->text, len) == 0 )
            mi_append_item (&prefix, mi);
        else
            mi_append_item (&substr, mi);
#pragma GCC diagnostic pop
    }
    if ( prefix.items != NULL )
        mi_append_list (&mi_matches, &prefix);
    if ( substr.items != NULL )
        mi_append_list (&mi_matches, &substr);

    mi_topleft = mi_matches.items;
    mi_select (mi_topleft, False);

    free (buf);

    /* layout items */
    return mi_layout_page ();
}

int
mi_read (const char *name)
{
     FILE *stream;
     int ret;

     stream = fopen (name, "r");
     if ( stream == NULL ) {
         error ("unable to open file %s for reading", name);
         return False;
     }

     ret = mi_read_file (stream);
     fclose (stream);
     return ret;
}

int
mi_read_file (FILE *file)
{
    char buf [BUFSIZ];
    MenuItem *mi, *mi_new;
    char *s;
    
    /* read each line from stdin and add it to the item list */
    for ( mi = (MenuItem *) mi_items.items; ; ) {
        if ( fgets (buf, BUFSIZ, file) == 0 )
            break;

        s = strchr (buf, '\n');
        if ( s != NULL )
            *s = '\0';

        s = s_dup (buf);
        if ( s == NULL )
            return False;

        mi->text = s;
        mi->flags = 0;
 
        /* increment $nelements and check allocated size */
        if ( ++mi_items.nelements != mi_items.alloc_size )
            mi++;
        else {
            mi_new = (MenuItem *) thunk_double_size (&mi_items, 0);
            if ( mi_new == NULL ) {
                free (mi->text);
                return False;
            }
            mi = mi_new + mi_items.nelements;
        }
    }
    return True;
}

int
mi_read_file_or_stdin (void)
{
    if ( a_items != NULL )
        return mi_read (a_items);

    return mi_read_file (stdin);
}

uint
mi_layout_page (void)
{
    uint x, row;
    MenuItem *item;

    item = mi_topleft;
    if ( item == NULL )
        return 1;
    
    /* prompt and input field */
    x = a_prompt_w + input.w;
    
    /* left arrow */
    if ( item->prev != NULL )
        x += drw_bh >> 1;

    /* calculate newlines and beginning of the next block */
    row = 0;
    do {
        /* let's assume the item is on the same row (~ x < drw_mw) */
        item->x = x;
        item->row = row;
        x += item->w;
        if ( x >= drw_mw ) {
            /* no, it belongs to next line */
            item->x = 0;
            item->row++;
            /* are we at the bottom? */
            if ( ++row != a_rows )
                /* not yet */
                x = item->w;
            else {
                /* yes, so let's check the space for the right arrow */
                if ( x - item->w + (drw_bh >> 1) >= drw_mw ) {
                    /* no space so the current item belongs to next page */
                    item = item->prev;
                    item->x = 0;
                    item->row = row;
                }
                break;
            }
        }
        item = item->next;
    } while ( item != NULL );

    /* $mi_next_page points to the next page */
    mi_next_page = item;
    return row;
}

void
mi_layout_bottom_right_rows (MenuItem* item, uint rows, int mw)
{
    for ( ; item != NULL; item = item->prev ) {
        mw -= item->w;
        if ( mw >= 0 )
            continue;
        
        /* previous row */
        if ( --rows == 0 ) {
            /* this item belongs to previous page */
            item->row = -1;
            /* there is no place for this item so use the next one */
            item = item->next;
            /* let's check the space for left arrow */
            mi_topleft = mw + item->w < drw_bh >> 1 ? item->next : item;
            return;
        }
        mw = drw_mw - item->w;
        if ( rows == 1 )
            /* prompt and input field */
            mw -= a_prompt_w + input.w;
    }

    mi_topleft = mi_matches.items;
}

int
mi_layout_last_page (void)
{
    /* last page ($mi_next_page is null) is little bit tricky: sometimes
     * few items are on the page, especially when the user scrolls down,
     * and this looks awful; we have to apply different approach in this
     * case: layout items from bottom right position; the $item is not the
     * first on the page but the result looks much better */ 
    if ( mi_next_page != NULL || mi_topleft->prev == NULL )
        return False;
        
    mi_layout_bottom_right (mi_matches.tail);
    return True;
}

void
mi_layout_top_right (MenuItem *item)
{
    uint mw;
   
    mw = drw_mw - a_prompt_w - input.w;
    if ( a_rows == 1 && item->next != NULL )
        mw -= drw_bh >> 1;

    mi_layout_bottom_right_rows (item, 1, mw);
    mi_layout_page ();
    if ( mi_layout_last_page () )
        return;
 
    /* the item should be at the top right position (row should be 0) we
     * need to shift the top left item and re-layou the page if this is
     * not the case */
    while ( item->row == 1 ) {
        mi_topleft = mi_topleft->next;
        mi_layout_page ();
    }
}

void
mi_layout_bottom_right (MenuItem *item)
{
    uint mw;
   
    mw = drw_mw;
    if ( a_rows == 1 )
        mw -= a_prompt_w + input.w;
    if ( item->next != NULL )
        mw -= drw_bh >> 1;
            
    mi_layout_bottom_right_rows (item, a_rows, mw);
    mi_layout_page ();
            
    while ( item->row == a_rows ) {
        mi_topleft = mi_topleft->next;
        mi_layout_page ();
    }
}

void
mi_layout_top_left (MenuItem *item)
{
    /* the $item should be the first on the page (top/left position) so
     * just layout next items until we reach next page ($mi_next_page) */
    mi_topleft = item;
    mi_layout_page ();
    mi_layout_last_page ();
}

void
mi_layout_bottom_left (MenuItem *item)
{
    uint mw;

    if ( a_rows == 1 ) {
        /* top and the bottom rows are the same when menu has only one
         * row */
        mi_layout_top_left (item);
        return;
    }
    mw = drw_mw;
    if ( a_rows == 2 )
        mw -= a_prompt_w + input.w;
        
    mi_layout_bottom_right_rows (item->prev, a_rows - 1, mw);
    mi_layout_page ();
    if ( mi_layout_last_page () )
        return;
   
    mw = a_rows - 2;
    while ( item->row == mw ) {
        /* the item should be at the bottom (last row) and on the left
         * but it's on the right and in previous row so let's shift
         * the top/left item and re-layout the page */
        mi_topleft = mi_topleft->prev;
        mi_layout_page ();
    }
} 

/* $sel is null: the function will select the item on the same row and at
 * the nearest horizontal position as the current selected item otherwise
 * the $sel item will be selected */
void
mi_ensure_visible (MenuItem *item, MenuItem *sel, Corner corner)
{
    uint sel_row, sel_middle_x;

    /* remember position and the row */
    if ( sel == NULL && mi_sel != NULL ) {
        sel_middle_x = mi_get_middle_x (mi_sel);
        sel_row = mi_sel->row;
    }
    switch ( corner ) {
        case BottomLeft:
            mi_layout_bottom_left (item);
            break;

        case TopRight:
            mi_layout_top_right (item);
            break;

        case BottomRight:
            mi_layout_bottom_right (item);
            break;

        default:  /* TopLeft */
            mi_layout_top_left (item);
            break;
    }
    
    /* select the item at the same position */
    if ( sel == NULL ) {
        if ( mi_sel == NULL )
            return;
        sel = mi_get_next_row (mi_topleft, sel_row);
        if ( sel == NULL )
            return;
        sel = mi_get_next_at_pos (sel, sel_middle_x);
    }
    mi_select (sel, True);
}
 
MouseStatus
mi_get_mouse_status (uint x, uint y, MenuItem **ret)
{
    uint row;
    MenuItem *item;
    MenuItem *prev;

    if ( mi_topleft == NULL )
        return MouseWindow;

    /* init */
    prev = NULL;
    row = y / drw_bh;

    /* first row? */
    if ( row == 0 ) {
        /* input field? */
        if ( x < a_prompt_w + input.w )
            return MouseInput;
        /* left arrow? */
        if ( x < mi_topleft->x )
            return MouseLeftArrow;
    } else if ( mi_next_page != NULL &&        /* items on the next page? */
                row == a_rows - 1 &&           /* last row? */
                x >= drw_mw - (drw_bh >> 1) )  /* check right arrow */
        return MouseRightArrow;

    /* menu items */
    item = mi_topleft;
    do {
        /* find the row */
        if ( item->row == row ) {
            /* we found the correct row; just ignore the first item */
            if ( prev != NULL && x < item->x ) {
                *ret = prev;
                return MouseMenuItem;
            }
            prev = item;
        } else if ( prev != NULL ) {
            /* we found the corrent row ($prev is not null) but this item
             * belongs to the next row */
            if ( x < prev->x + prev->w ) {
                *ret = prev;
                return MouseMenuItem;
            }
            return MouseWindow;
        }
        /* next item */
        item = item->next;
    } while ( item != NULL );

    return MouseWindow;
}

int
mi_handle_click (uint x, uint y)
{
    MenuItem *item;
    MouseStatus mstat;

    mstat = mi_get_mouse_status (x, y, &item);
    if ( mstat == MouseWindow )
        return True;
    
    switch ( mstat ) {
        case MouseLeftArrow:
            mi_ensure_visible (mi_topleft->prev, NULL, BottomRight);
            break;

         case MouseRightArrow:
            mi_ensure_visible (mi_next_page, NULL, TopLeft);
            break;

         case MouseInput:
            mi_sel = NULL;
            break;

         default:
            mi_sel = item;
            break;
   }

   return drw_draw_menu ();
}

void
mi_update_geometry (void)
{
    MenuItem *mi;
    uint w, max_w, i;

    max_w = drw_mw / 3;
    input.w = 0;

    for ( i = mi_items.nelements, mi = (MenuItem *) mi_items.items; i != 0; i--, mi++ ) {
        /* text width */
        w = drw_get_text_width (mi->text);

        /* menu item width */
        w = mi_width (w);
        if ( w > max_w )
            w = max_w;

        mi->w = w;

        /* input width */
        if (input.w < w)
            input.w = w;
    }
}

int
mi_key_press (KeySym ksym, uint state, char *buf, uint len)
{
    switch ( ksym ) {
        case XK_Escape:
            /* terminate the app */
            return 1;

        case XK_Delete:
            if ( mi_sel->flags & MenuItemDisabled )
                mi_sel->flags &= ~MenuItemDisabled;
            else
                mi_sel->flags |= MenuItemDisabled;
            break;
            
        case XK_Home:
            if ( mi_sel == mi_matches.items )
                return 0;
            /* no previous page? */
            if ( mi_topleft == mi_matches.items ) {
                /* select top/left item */
                mi_select (mi_matches.items, True);
                break;
            }
            /* scroll page */
            mi_ensure_visible (mi_matches.items, mi_matches.items, TopLeft);
            break;
    
        case XK_End:
            if ( mi_sel == mi_matches.tail )
                return 0;
            /* no next page? */
            if ( mi_next_page == NULL ) {
                /* select last item */
                mi_select (mi_matches.tail, True);
                break;
            }
            /* scroll page */
            mi_ensure_visible ( mi_matches.tail, mi_matches.tail, BottomRight);
            break;
    
        case XK_Left:
          {  
            int scroll;

            if ( mi_sel == mi_matches.items ) {
                /* focus has the first item in the list:
                 * set focus to the input */
                mi_sel->flags &= ~MenuItemHilighted;
                break;
            }
            /* previous page? */
            scroll = mi_sel == mi_topleft;
            mi_select (mi_sel->prev, True);
            if ( scroll )
                mi_ensure_visible (mi_sel, mi_sel, TopRight);
            break;
          }

        case XK_Right:
            if ( mi_sel == mi_matches.tail )
                return 0;
            /* select next item */
            mi_select (mi_sel->next, True);
            /* next page? */
            if ( mi_sel == mi_next_page )
                mi_ensure_visible (mi_next_page, mi_next_page, BottomLeft);
            break;

        case XK_Up:
          { 
            MenuItem *item;

            /* first row? */ 
            if ( mi_sel->row == 0 ) {
                /* previous page? */
                if ( mi_topleft == mi_matches.items )
                    return 0;
                /* scroll: try to layout the items so $mi_topleft->prev
                 * item is the last in the first row */
                mi_ensure_visible (mi_topleft->prev, NULL, TopRight);
                break;
            }
            /* select item in previous row */
            item = mi_get_prev_row (mi_sel, 1);
            item = mi_get_prev_at_pos (item, mi_get_middle_x (mi_sel));
            mi_select (item, True);
            break;
          }

        case XK_Down:
          {
            MenuItem *item;

            /* last row? */
            if ( mi_sel->row == a_rows - 1 ) {
                /* next page? */
                if ( mi_next_page == NULL )
                    return 0;
                /* scroll: try to layout the items so the $mi_next_page
                 * item is the first in the last row */
                mi_ensure_visible (mi_next_page, NULL, BottomLeft);
                break;
            }
            /* select item in next row */
            item = mi_get_next_row (mi_sel, 1);
            if ( item == NULL )
                return 0;
            item = mi_get_next_at_pos (item, mi_get_middle_x (mi_sel));
            mi_select (item, True);
            break;
          }
    
        case XK_Next:
            /* next page? */
            if ( mi_next_page != NULL )
                mi_ensure_visible (mi_next_page, NULL, TopLeft);
            else
                mi_select (mi_matches.tail, True);
            break;

        case XK_Prior:
            /* previous page? */
            if ( mi_topleft != mi_matches.items )
                mi_ensure_visible (mi_topleft->prev, NULL, BottomRight);
            else if ( mi_topleft != mi_sel )
                mi_select (mi_topleft, True);
            break;

        case XK_Return:
        case XK_KP_Enter:
            verbose_s (mi_sel->text);
            verbose_newline ();
            if ( (state & ControlMask) == 0 )
               return 1;
            mi_sel->flags |= MenuItemDisabled;
            break;
    
        case XK_Tab:
            mi_sel->flags &= ~MenuItemHilighted;
            break;

        case XK_space:
            /* CTRL? */
            if ( (state & ControlMask) == 0 )
                goto insert;
            /* yes */
            if ( input_set (mi_sel->text, -1) == -1 )
                return -1;
            break;

        default:
            if ( iscntrl (*buf) )
                return 0;
insert:     
            mi_sel->flags &= ~MenuItemHilighted;
            if ( input_insert (buf, len) == -1 )
                return -1;
            break;
    }
    if ( !drw_draw_menu () )
        return -1;
    return 0;
}
