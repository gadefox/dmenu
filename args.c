/* See LICENSE file for copyright and license details. */

#include "args.h"
#include "verbose.h"
#include "util.h"
#include "strutil.h"


/* rows value must be between LINES_MIN and LINES_MAX */
#define ROWS_MIN  1
#define ROWS_MAX  30


const char version_arg[] = "version";
const char help_arg[]    = "help";
const char fast_arg[]    = "fast";
const char casei_arg[]   = "casei";
const char rows_arg[]    = "rows";
const char mon_arg[]     = "mon";
const char prompt_arg[]  = "prompt";
const char font_arg[]    = "font";
const char nfg_arg[]     = "nfg";
const char nbg_arg[]     = "nbg";
const char sfg_arg[]     = "sfg";
const char sbg_arg[]     = "sbg";
const char win_arg[]     = "winid";
const char items_arg[]   = "items";


Flags a_flags = FlagAlignTop;
int a_rows = 3;
int a_mon = -1;
const char *a_prompt = "Contains:";
char *a_font = "Liberaton Mono:pango:monospace:pixelsize=14:antialias=true:autohint=true";
const char *a_color [ColorMax] = { "#263238", "#DFDFDF", "#82888B", "#FFFFFF", "#A4A900" };

char *a_items = NULL;
Window a_winid = None;
uint a_prompt_w;


/*
 * Args
 */

void
args_copy_remaining (Args *dst, Args *src)
{
    Args copy, cur;

    cur.count = src->count;
    cur.v     = src->v;

    copy.count = dst->count;
    copy.v     = dst->v;

    /* copy counting arguments */
    *copy.v++ = src->arg;
    copy.count++;

    while ( cur.count != 0 ) {
        *copy.v++ = *cur.v++;
        cur.count--;
        copy.count++;
    }

    dst->count = copy.count;
}

ArgsStatus
args_fetch_next (Args *args, const char *argname, int no_parse )
{
    char *name;
    char *value;
    ArgsStatus stat;

    /* counting arguments? */
    if ( args->count == 0 ) {
        if ( argname != NULL )
            error ( msg_arg_missing, argname);
        return ArgsEnd;
    }

    /* get next */
    args->count--;

    name = *args->v++;
    args->arg = name;

    if ( no_parse )
        return ArgsWord;

    /* check '--' and '-' */
    if ( *name == '-' ) {
        if ( *++name == '-' ) {
            /* parse name/value pair */
            value = strchr (++name, '=');
            if ( value == NULL )
                args->value = NULL;
            else {
                *value++ = '\0';
                args->value = value;
            }
            stat = ArgsDouble;
        } else
            stat = ArgsSingle;
    } else
        stat = ArgsWord;

    args->name = name;
    return stat;
}

/*
 * Run params
 */

int
args_parse (Args *args)
{
    Args cur, copy;
    ArgsStatus stat;
    char c;
    Window w;

    /* init */
    cur.count = args->count;
    cur.v     = args->v;
    
    copy.count = 0;
    copy.v     = cur.v;
   
    /* handle arguments */
    for ( ;; ) {
        stat = args_fetch_next (&cur, NULL, False);
        if ( stat == ArgsEnd )
            break;
        
        if ( stat == ArgsDouble ) {
            /* -- */
            if ( *cur.name == '\0' && cur.value == NULL ) {
                /* copy remaining arguments */
                args_copy_remaining (&copy, &cur);
                break;
            }
            /* --version */
            if ( strcmp (cur.name, version_arg) == 0 ) {
                a_flags |= FlagVersion;
                continue;
            }
            /* --help */
            if ( strcmp (cur.name, help_arg) == 0 ) {
                a_flags |= FlagHelp;
                continue;
            }
            /* --fast */
            if ( strcmp (cur.name, fast_arg) == 0) {
                a_flags |= FlagFast;
                continue;
            }
            /* --casei */
            if ( strcmp (cur.name, casei_arg) == 0) {
                case_insensitive ();
                continue;
            }
            /* --rows */
            if ( strcmp (cur.name, rows_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                /* parse # */
                s_parse_uint (cur.value, &a_rows);
                if ( a_rows == -1 ) {
                    error (msg_invalid_number, rows_arg, cur.value);
                    return False;
                }
                if ( !BETWEEN (a_rows, ROWS_MIN, ROWS_MAX) ) {
                    error ("value for rows must be between %d and %d", ROWS_MIN, ROWS_MAX);
                    return False;
                }
                continue;
            }
            /* --mon */
            if ( strcmp (cur.name, mon_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                /* parse # */
                s_parse_uint (cur.value, &a_mon);
                if ( a_mon == -1 ) {
                    error (msg_invalid_number, mon_arg, cur.value);
                    return False;
                }
                continue;
            }
            /* --prompt */
            if ( strcmp (cur.name, prompt_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_prompt = cur.value;
                continue;
            }
            /* --font */
            if ( strcmp (cur.name, font_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_font = cur.value;
                continue;
            }
            /* --items */
            if ( strcmp (cur.name, items_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_items = cur.value;
                continue;
            }
            /* --nfg */
            if ( strcmp (cur.name, nfg_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_color [ColorItem] = cur.value;
                continue;
            }
            /* --nbg */
            if ( strcmp (cur.name, nbg_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_color [ColorWindow] = cur.value;
                continue;
            }
            /* --sfg */
            if ( strcmp (cur.name, sfg_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_color [ColorText] = cur.value;
                continue;
            }
            /* --sbg */
            if ( strcmp (cur.name, sbg_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                a_color [ColorSelected] = cur.value;
                continue;
            }
            /* --winid */
            if ( strcmp (cur.name, win_arg) == 0 ) {
                if ( cur.value == NULL ) {
                    error (msg_arg_missing, cur.arg);
                    return False;
                }
                
                sscanf (cur.value, "0x%lx", &w);
                if ( w == None )
                    sscanf (cur.value, "%lu", &w);
                if ( w == None ) {
                    error (msg_invalid_winid, cur.value);
                    return False;
                }
                a_winid = w;
                continue;
            }
            /* unknown argument */
            error (msg_arg_unknown, cur.arg);
            return False;
        }
        
        if (stat == ArgsSingle ) {
            if ( *cur.name == '\0' ) {
                /* copy remaining arguments */
                args_copy_remaining (&copy, &cur);
                break;
            }

            for ( c = *cur.name; c != '\0'; c = *++cur.name ) {
                /* -i ~ --casei */
                if ( c == 'i' )
                    case_insensitive ();
                /* -f ~ --fast */
                else if ( c == 'd' )
                    a_flags |= FlagFast;
                /* -h ~ --help */
                else if ( c == 'h' )
                    a_flags |= FlagHelp;
                /* -v ~- --version */
                else if ( c == 'v' )
                    a_flags |= FlagVersion;
                /* unknown argument */
                else {
                    error (msg_arg_unknown_char, c);
                    return False;
                }
            }
        } else {
            /* copy argument */
            *copy.v++ = cur.arg;
            copy.count++;
        }
    }

    args->count = copy.count;
    return True;
}
