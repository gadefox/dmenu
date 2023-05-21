/* See LICENSE file for copyright and license details. */

#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <X11/Xatom.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#include "dmenu.h"
#include "util.h"
#include "menuitem.h"
#include "verbose.h"
#include "args.h"
#include "input.h"


//#define DEBUG

Window d_parent, d_win = None;
Drw d_drw = { NULL, None, None, { NULL, 0 } };
XftColor d_color [ColorMax];
Atom d_clip, d_utf8;

static XIC d_xic = NULL;
static XIM d_xim = NULL;


static int
grab_focus (void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000  };
    Window focuswin;
    uint i;
    int revertwin;

    for ( i = 0; i < 100; i++ ) {
        XGetInputFocus (d_drw.dpy, &focuswin, &revertwin);
        if (focuswin == d_win)
            return True;

        XSetInputFocus (d_drw.dpy, d_win, RevertToParent, CurrentTime);
        nanosleep (&ts, NULL);
    }
    error ("cannot grab focus");
    return False;
}

static void
ungrab_keyboard (void)
{
    XEvent ev;
    
    XUngrabKeyboard (d_drw.dpy, CurrentTime);
    while ( XPending (d_drw.dpy) )
        XNextEvent (d_drw.dpy, &ev);
    XFlush (d_drw.dpy);
}

static int
grab_keyboard (void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
    uint i;

//    return 1;
    if ( a_winid != None )
        return True;

    /* try to grab keyboard, we may have to wait for another process to ungrab */
    for ( i = 0; i < 1000; i++ ) {
        if (XGrabKeyboard (d_drw.dpy, DefaultRootWindow (d_drw.dpy),
                   True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
            return True;

        nanosleep (&ts, NULL);
    }
    error ("cannot grab keyboard");
    return False;
}

static int
handle_key_press (XKeyEvent *ev)
{
    char buf [32];
    uint len;
    KeySym ksym;
    Status status;

#ifdef DEBUG
    ungrab_keyboard ();
#endif

    len = XmbLookupString (d_xic, ev, buf, sizeof (buf), &ksym, &status);
    switch ( status ) {
        default:  /* XLookupNone, XBufferOverflow */
            return 0;

        case XLookupChars:
            if ( iscntrl (*buf) )
                return 0;
            if ( input_insert (buf, len) == -1 )
                return -1;
            goto draw;

        case XLookupKeySym:
        case XLookupBoth:
            break;
    }
    /* is menu item hilighted? */
    if ( mi_is_focused () )
        return mi_key_press (ksym, ev->state, buf, len);
    
    return input_key_press (ksym, ev->state, buf, len);

draw:
    if ( !drw_draw_menu () )
        return -1;
    return 0;
}

static int
paste (void)
{
    char *p, *q;
    int di, ret;
    unsigned long dl;
    Atom da;

    /* we have been given the current selection, now insert it into input */
    if ( XGetWindowProperty (d_drw.dpy, d_win, d_utf8, 0, (BUFSIZ >> 2) + 1,
            False, d_utf8, &da, &di, &dl, &dl, (byte **) &p) == Success && p != NULL ) {
        q = strchr (p, '\n');
        if ( q != NULL )
            ret = input_insert (p, q - p);
        else {
            di = strlen (p);
            ret = input_insert (p, di);
        }
        XFree (p);
        if ( ret == -1 )
            return False;
    }

    return drw_draw_menu ();
}

static int
run (void)
{
    XEvent ev;

    while ( !XNextEvent (d_drw.dpy, &ev) ) {
        if ( XFilterEvent (&ev, d_win) )
            continue;

        switch (ev.type) {
            case DestroyNotify:
                if (ev.xdestroywindow.window != d_win)
                    break;
                return EXIT_SUCCESS;

            case Expose:
                if (ev.xexpose.count == 0)
                    drw_map (0, 0, drw_mw, drw_mh);
                break;

            case FocusIn:
                /* regrab focus from parent window */
                if ( ev.xfocus.window != d_win && !grab_focus () )
                    return EXIT_FAILURE;
                break;

            case KeyPress:
              {
                int ret;
               
                ret = handle_key_press (&ev.xkey);
#ifdef DEBUG
                grab_keyboard ();
#endif
                if ( ret == -1 )
                    return EXIT_FAILURE;
                if ( ret == 1 )
                    return EXIT_SUCCESS;
                break;
              }

            case ButtonPress:
                if ( ev.xbutton.button == Button1 &&
                     !mi_handle_click (ev.xbutton.x, ev.xbutton.y) )
                    return EXIT_FAILURE;
                break;

            case SelectionNotify:
                if ( ev.xselection.property == d_utf8 && !paste () )
                    return EXIT_FAILURE;
                break;

            case VisibilityNotify:
                if ( ev.xvisibility.state != VisibilityUnobscured )
                    XRaiseWindow (d_drw.dpy, d_win);
                break;
        }
    }
    return EXIT_SUCCESS;
}

#ifdef XINERAMA

static XineramaScreenInfo *
get_max_area (Window w, XineramaScreenInfo *info, uint size)
{
    Window dw, pw;
    Window *dws;
    uint du;
    XWindowAttributes wa;
    int area, area_max = 0;
    XineramaScreenInfo *pmax = NULL;

    if ( w == d_drw.root || w == PointerRoot || w == None )
        return NULL;

    /* find top-level window containing current input focus */
    do {
        pw = w;
        if ( XQueryTree( d_drw.dpy, pw, &dw, &w, &dws, &du) && dws != NULL )
            XFree (dws);
    } while ( w != d_drw.root && w != pw );

    /* find xinerama screen with which the window intersects most */
    if ( !XGetWindowAttributes (d_drw.dpy, pw, &wa) )
        return NULL;
      
    while ( size-- != 0 ) {  
        area = intersect_area (wa.x, wa.y, wa.width, wa.height, info->x_org, info->y_org, info->width, info->height);
        if ( area_max < area ) {
            area_max = area;
            pmax = info;
        }
        info++;
    }
    return pmax;
}

static XineramaScreenInfo *
get_intersect (XineramaScreenInfo *info, uint size)
{
    Window dw;
    int x, y, di;
    uint du;

    /* no focused window is on screen, so use pointer location instead */
    if ( !XQueryPointer (d_drw.dpy, d_drw.root, &dw, &dw, &x, &y, &di, &di, &du) )
        return NULL;
  
    while ( size-- != 0 ) {
        if ( intersect (x, y, 1, 1, info->x_org, info->y_org, info->width, info->height) )
            return info;
        info++;
    }
    return NULL; 
}

static int
xinerama (int *ret_x, int *ret_y)
{
    XineramaScreenInfo *info, *sel;
    Window w;
    int di, n;

    if ( d_parent != d_drw.root )
        return False;
    
    info = XineramaQueryScreens (d_drw.dpy, &n);
    if ( info == NULL )
        return False;

    XGetInputFocus (d_drw.dpy, &w, &di);

    if ( IS_INDEX (a_mon, n) )
        sel = info + a_mon;  /* ~ &info [a_mon] */
    else if ( n > 1 )
        if ( (sel = get_max_area (w, info, n)) == NULL &&
             (sel = get_intersect (info, n)) == NULL )
            sel = info;
    } else
        sel = info;

    if ( (a_flags & FlagAlignTop) == 0 )
        sel->y_org += sel->height;
    
    *ret_x = sel->x_org;
    *ret_y = sel->y_org;
    drw_mw = sel->width;

    XFree (info);
    return True;
}

#endif  /* XINERAMA */

static int
setup (void)
{
    uint x, y;
    uint du;
    XSetWindowAttributes swa;
    Window w, dw;
    Window *dws, *cws;
    XWindowAttributes wa;
    Fnt *fnt;
    XClassHint ch;
   
    /* init */
    ch.res_name  = (char *) prog_name;
    ch.res_class = (char *) prog_name;

    drw_colors_init (d_color, ColorMax);
 
    if ( !setlocale (LC_CTYPE, "") || !XSupportsLocale ())
        warn ("no locale support");
   
    /* menuitems */
    if ( !mi_init () || !input_init ()  || !drw_init () )
        return False;

    d_drw.dpy = XOpenDisplay (NULL);
    if ( d_drw.dpy == NULL ) {
        error ("can not open display");
        return False;
    }
    d_drw.screen = DefaultScreen (d_drw.dpy);
    d_drw.root = RootWindow (d_drw.dpy, d_drw.screen);
    if ( a_winid == None )
        d_parent = d_drw.root;

    if ( !drw_create () )
        return False;

    /* init appearance */
    if ( !drw_colors_create (d_color, a_color, ColorMax) ) 
        return False;
   
    fnt = drw_fnt_open_name (a_font);
    if ( fnt == NULL )
        return False;

    /* calculate menu geometry */
    drw_padding_x = fnt->h >> 1;
    drw_bh = fnt->h + (drw_padding_y << 1);

    d_clip = XInternAtom (d_drw.dpy, "CLIPBOARD", False);
    d_utf8 = XInternAtom (d_drw.dpy, "UTF8_STRING", False);

#ifdef __OpenBSD__
    if ( pledge ("stdio rpath", NULL) == -1 ) {
        error ("pledge");
        return False;
    }
#endif

    if ( (a_flags & FlagFast) && !isatty (0) ) {
        if ( !grab_keyboard () || !mi_read_file_or_stdin () )
            return False;
    } else {
        if ( !mi_read_file_or_stdin () || !grab_keyboard () )
            return False;
    }

#ifdef XINERAMA
    if ( !xinerama (&x, &y) )
#endif
    {
        if ( !XGetWindowAttributes (d_drw.dpy, d_parent, &wa) ) {
            error ("could not get embedding window attributes: 0x%lx", d_parent);
            return False;
        }
        x = 0;
        y = a_flags & FlagAlignTop ? 0 : wa.height;
        drw_mw = wa.width;
    }

    /* menu items geometry */
    a_prompt_w = mi_width (drw_get_text_width (a_prompt));
    mi_update_geometry ();
    
    du = mi_match ();
    mi_sel = mi_topleft;
    mi_sel->flags |= MenuItemHilighted;
    
    drw_mh = du * drw_bh;
    if ( (a_flags & FlagAlignTop) == 0 )
        y -= drw_mh;

    /* create menu window */
    swa.override_redirect = True;
    swa.background_pixel = d_color [ColorWindow].pixel;
    swa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | VisibilityChangeMask;

    d_win = XCreateWindow (d_drw.dpy, d_parent, x, y, drw_mw, drw_mh, 0, CopyFromParent,
           CopyFromParent, CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);
    XSetClassHint (d_drw.dpy, d_win, &ch);

    /* input methods */
    d_xim = XOpenIM (d_drw.dpy, NULL, NULL, NULL);
    if ( d_xim == NULL ) {
        error ("could not open input device");
        return False;
    }
    d_xic = XCreateIC (d_xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                       XNClientWindow, d_win, XNFocusWindow, d_win, NULL);
    XMapRaised (d_drw.dpy, d_win);

    if ( a_winid != None ) {
        XSelectInput (d_drw.dpy, d_parent, FocusChangeMask | SubstructureNotifyMask);

        if ( XQueryTree (d_drw.dpy, d_parent, &dw, &w, &dws, &du) && dws != NULL ) {
            for ( cws = dws, w = *cws; du != 0 && w != d_win; du--, w = *++cws )
                XSelectInput (d_drw.dpy, w, FocusChangeMask);
            XFree (dws);
        }
        grab_focus ();
    }
    
    drw_resize (drw_mw, drw_mh);
    return drw_draw_menu ();
}

void
done (void)
{
    /* ungrab keyboard */
    ungrab_keyboard ();

    /* input */
    input_free ();

    /* menuitems */
    mi_free ();

    /* colors */
    drw_colors_destroy (d_color, ColorMax);

    /* display etc. */
    drw_free ();
    
    if (d_xic != NULL)
        XDestroyIC (d_xic);

    if (d_xim != NULL)
        XCloseIM (d_xim);
}

int
main (int argc, char *argv[])
{
    Args args;

    prog_name = get_prog_name (*argv);
    args.count = --argc;
    args.v     = ++argv;

    if ( !args_parse (&args) )
        return EXIT_FAILURE;
  
    if ( a_flags & FlagHelp ) {
        usage ();
        return EXIT_SUCCESS;
    }
    if ( a_flags & FlagVersion ) {
        version ();
        return EXIT_SUCCESS;
    }
    if ( setup () )
        run();

    done ();
    return EXIT_SUCCESS;
}
