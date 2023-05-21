/* See LICENSE file for copyright and license details. */

#include <X11/Xatom.h>
#include <ctype.h>
#include "input.h"
#include "strutil.h"
#include "util.h"
#include "verbose.h"
#include "menuitem.h"
#include "drw.h"
#include "dmenu.h"


/*
 * Characters not considered part of a word while deleting words
 * for example: " /?\"&[]"
 */
const char input_delim [] = " ";

Thunk input_tok = { NULL };
Input input = { { NULL } };

int
input_init (void)
{
    if ( thunk_create (&input.t, 64, sizeof (char)) == NULL )
        return False;

    /* input field */
    *input_begin = '\0';
    input.t.nelements = 1;
    input.cursor = input_begin;

    return thunk_create (&input_tok, 32, sizeof (char **)) != NULL;
}

void
input_free (void)
{
    thunk_free (&input.t);
    thunk_free (&input_tok);
}

char *
input_update_tokens (void)
{
    char *buf, *s;
    char **tokv;

    buf = s_dup (input_begin);
    if ( buf == NULL )
        return NULL;

    /* separate input text into tokens to be matched individually */
    for ( input_tok.nelements = 0, tokv = input_tok_v, s = strtok (buf, " ");
          s != NULL;
          s = strtok (NULL, " ") ) {
        *tokv = s;

        /* increment $nelements and check allocated size */
        if ( ++input_tok.nelements != input_tok.alloc_size )
            tokv++;
        else {
            tokv = (char **) thunk_double_size (&input_tok, 0);
            if ( tokv == NULL ) {
                free (buf);
                return NULL;
            }
            tokv += input_tok.nelements;
        }
    }
    return buf;
}

int
input_check_tokens (char *value)
{
    uint idx;
    char **tok;

    for ( idx = input_tok.nelements, tok = input_tok_v; idx != 0; idx--, tok++ ) {
        if ( x_strstr (value, *tok) == NULL )
            /* not all tokens match */
            return False;
    }
    return True;
}

int
input_insert (const char *str, int n)
{
    uint total, offset;

    offset = input.cursor - input_begin;
    total = input.t.nelements + n;
    
    if ( total > input.t.alloc_size ) {
        /* reallocate buffer */
        if ( thunk_double_size (&input.t, total) == NULL )
            return -1;

        input.cursor = input_begin + offset;
    }

    /* move existing text out of the way, insert new text, and update cursor */
    memmove (input.cursor + n, input.cursor, (input.t.nelements - offset) * sizeof (char));
    if ( str != NULL )
       memcpy (input.cursor, str, n * sizeof (char));
    input.cursor += n;
    input.t.nelements = total;  /* ~ input.t.nelements += n */

    return mi_match ();
}

int
input_set (const char *str, int n)
{
    if ( n == -1 )
        n = strlen (str);

    /* reallocate buffer */
    if ( n > input.t.alloc_size &&
         thunk_double_size (&input.t, n) == NULL )
        return -1;

    memcpy (input.t.items, str, n * sizeof (char));
    input_begin [n] = '\0';
    input.cursor = input_begin + n;
    input.t.nelements = n + 1;

    return mi_match ();
}

char *
input_next_rune_s (char *s)
{
    /* return next location of next utf8 rune */
    for ( s++; (*s & 0xc0) == 0x80; s++ )
        ;  /* NOP */
    return s;
}

char *
input_prev_rune_s (char *s)
{
    /* return location of next utf8 rune in the given direction (+1 or -1) */
    for ( s--; s > input_begin && (*s & 0xc0) == 0x80; s-- )
        ; /* NOP */
    return s;
}

int
input_next_rune (void)
{
    char *rune;

    rune = input_next_rune_s (input.cursor);
    return rune - input.cursor;
}

int
input_prev_rune (void)
{
    char *rune;

    rune = input_prev_rune_s (input.cursor);
    return rune - input.cursor;
}

void
input_move_next_word_edge (void)
{
    char c;

    /* move cursor to the end of the word */
    for ( ;; ) {
        c = *input.cursor;
        if ( c == '\0' )
            break;
        if ( strchr (input_delim, c) == NULL )
            break;

        input.cursor = input_next_rune_s (input.cursor);
    }
    for ( ;; ) {
        c = *input.cursor;
        if ( c == '\0' )
            break;
        if ( strchr (input_delim, c) != NULL )
            break;

        input.cursor = input_next_rune_s (input.cursor);
    }
}

void
input_move_prev_word_edge (void)
{
    char *rune;

    /* move cursor to the start of the word */
    rune = input.cursor;

    while ( input.cursor > input_begin ) {
        rune = input_prev_rune_s (input.cursor);
        if ( strchr (input_delim, *rune) == NULL )
            break;

        input.cursor = rune;
    }

    while ( input.cursor > input_begin ) {
        rune = input_prev_rune_s (input.cursor);
        if ( strchr (input_delim, *rune) != NULL )
            break;

        input.cursor = rune;
    }
}

int
input_delete_word (void)
{
    char *rune;
    char c;

    /* word beginning */
    rune = input.cursor;

    while ( rune > input_begin ) {
        rune = input_prev_rune_s (rune);
        if ( strchr (input_delim, *rune) != NULL )
            break;
    }
    /* word ending */
    for ( ;; ) {
        c = *input.cursor;
        if ( c == '\0' )
            break;
        if ( strchr (input_delim, c) != NULL )
            break;

        input.cursor = input_next_rune_s (input.cursor);
    }
    /* delete word */
    return input_insert (NULL, rune - input.cursor);
}

int
input_key_press (KeySym ksym, uint state, char *buf, uint len)
{
    switch ( ksym ) {
        case XK_Escape:
            if ( mi_sel == NULL )
                return 1;
            /* unfocus */
            mi_sel->flags |= MenuItemHilighted;
            break;
            
        case XK_Delete:
          {
            char *cur;
                
            /* CTRL? */
            if ( state & ControlMask ) {
                /* yes: delete the whole content */
                if ( input.cursor == input_begin &&
                     *input.cursor == '\0' )
                    return 0;
                    
                input.cursor = input_begin;
                *input.cursor = '\0';
                input.t.nelements = 1;
                    
                if ( mi_match () == -1 )
                    return -1;
                break;
            }
            /* no: delete next character */
            if ( *input.cursor == '\0' )
                return 0;
                
            cur = input.cursor;
            input.cursor = input_next_rune_s (input.cursor);
            if ( input_insert (NULL, cur - input.cursor) == -1 )
                return -1;
            break;
          }
            
        case XK_BackSpace:
          {
            int rune;
                
            /* focus has the input: delete previous character */
            if ( input.cursor == input_begin )
                return 0;

            rune = input_prev_rune ();
            if ( input_insert (NULL, rune) == -1 )
                return -1;
            break;
          }
            
        case XK_Home:
            /* CTRL? */
            if ( state & ControlMask ) {
                /* delete left */
                if ( input_insert (NULL, input_begin - input.cursor) == -1 )
                    return -1;
                break;
            }
            /* no: is the cursor at the beginning? */
            if ( input.cursor == input_begin )
                return 0;
            /* no: move the cursor to the beginning */
            input.cursor = input_begin;
            break;

        case XK_End:
            /* CTRL? */
            if ( state & ControlMask ) {
                /* delete right */
                *input.cursor = '\0';
                input.t.nelements = input.cursor - input_begin + 1;
                if ( mi_match () == -1 )
                    return -1;
                break;
            }
            /* not: is the cursor at the end? */
            if ( *input.cursor == '\0')
                return 0;
            /* no: move the cursor to the end */
            input.cursor = input_end;
            break;

        case XK_Left:
            /* CTRL? */
            if ( state & ControlMask ) {
                input_move_prev_word_edge ();
                break;
            }
            /* is the cursor at the beginning? */
            if ( input.cursor == input_begin )
                return 0;
            /* no: move the cursor to the left */
            input.cursor = input_prev_rune_s (input.cursor);
            break;

        case XK_Right:
            /* CTRL? */
            if ( state & ControlMask ) {
                input_move_next_word_edge ();
                break;
            }
            /* is the cursor at the end? */
            if ( *input.cursor == '\0' ) {
                /* yes: set focus to menu item */
                if ( mi_sel == NULL )
                    return 0;
                mi_sel->flags |= MenuItemHilighted;
                break;
            }
            /* no: move the cursor to the right */
            input.cursor = input_next_rune_s (input.cursor);
            break;

        case XK_Up:
        case XK_Down:
        case XK_Tab:
            if ( mi_sel == NULL )
                return 0;
            mi_sel->flags |= MenuItemHilighted;
            break;
    
        case XK_Return:
        case XK_KP_Enter:
            verbose_s (input_begin);
            verbose_newline ();
            return (state & ControlMask) == 0;

        case XK_w:
            /* CTRL? */
            if ( (state & ControlMask) == 0 )
                goto insert;
            /* yes: delete word */
            if ( input_delete_word () == -1 )
                return -1;
            break;

        case XK_y:
        case XK_Y:
            /* CTRL? */
            if ( (state & ControlMask) == 0 )
                goto insert;
            /* paste selection */
            XConvertSelection (d_drw.dpy, state & ShiftMask ? d_clip : XA_PRIMARY,
                               d_utf8, d_utf8, d_win, CurrentTime);
            break;

        default:
            if ( iscntrl (*buf) )
                return 0;
insert:            
            if ( input_insert (buf, len) == -1 )
                return -1;
            break;
    }
    if ( !drw_draw_menu () )
        return -1;
    return 0;
}
