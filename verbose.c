/* See LICENSE file for copyright and license details. */

#include "verbose.h"
#include "typedef.h"
#include "strutil.h"


const char *prog_name;

const char msg_arg_unknown[]      = "unrecognized argument: %s";
const char msg_arg_unknown_char[] = "unrecognized argument: -%c";
const char msg_arg_missing[]      = "%s requires an argument";
const char msg_invalid_number[]   = "%s: invalid #: %s";
const char msg_invalid_winid[]    = "window: invalid id # %s";
const char msg_out_of_memory[]    = "out of memory";


void
verbose_color_begin (FILE *file, VerboseColor color)
{
    fprintf (file, "\033[%d;1m", color);
}

void
verbose_color_end (FILE *file)
{
    fputs ("\033[0m", file);
}

void verbose_color (FILE *file, const char *str, VerboseColor color)
{
    verbose_color_begin (file, color);
    fputs (str, file);
    verbose_color_end (file);
}

void
verbose_prefix (FILE *file, const char *prefix)
{
    /* "$prefix: " */
    fputs (prefix, file);
    fputc (':', file);
    fputc (' ', file);
}

void
verbose_color_prefix (FILE *file, const char *prefix, VerboseColor color)
{
    verbose_color_begin (file, color);
    verbose_prefix (file, prefix);
    verbose_color_end (file);
}

void
verbose (FILE *file, const char *prefix, VerboseColor color, const char *format, va_list params)
{
    verbose_prefix (file, prog_name);
    verbose_color_prefix (file, prefix, color);
    vfprintf (file, format, params);
    fputc ('\n', file);
}

void
verbose_string (const char *name, const char *val)
{
    if ( name != NULL )
        verbose_color_prefix (stdout, name, VerboseCyan);

    verbose_s (val);
    verbose_newline ();
}

void
verbose_v (const char **v, int size)
{
    const char *s;
    
    if ( size == -1 ) {
        for ( s = *v; s != NULL; s = *++v ) {
            verbose_s (s);
            verbose_newline ();
        }
    } else {
         while ( size-- != 0 ) {
            verbose_s (*v++);
            verbose_newline ();
        }
    }
}

void
verbose_spaces (unsigned int count)
{
    while (count-- != 0 )
        verbose_c (' ');
}

void
help (const char *format, ...)
{
    va_list params;

    va_start (params, format);
    verbose (stdout, "HELP", VerboseMagenda, format, params);
    va_end (params);
}

void
warn (const char *format, ...)
{
    va_list params;

    va_start (params, format);
    verbose (stderr, "WARNING", VerboseYellow, format, params);
    va_end (params);
}

void
error (const char *format, ...)
{
    va_list params;

    va_start (params, format);
    verbose (stderr, "ERROR", VerboseRed, format, params);
    va_end (params);
}

void
info (const char *format, ...)
{
    va_list params;

    va_start (params, format);
    verbose (stdout, "INFO", VerboseGreen, format, params);
    va_end (params);
}

void
verbose_d (int value)
{
    char szval [MAX_NUM_SIZE];

    s_int (szval, value);
    verbose_s (szval);
}

void
verbose_color_int (int value, VerboseColor color)
{
    verbose_color_begin (stdout, color);
    verbose_d (value);
    verbose_color_end (stdout);
}

void
verbose_u (unsigned int value)
{
    char szval [MAX_NUM_SIZE];

    s_uint (szval, value);
    verbose_s (szval);
}

void
verbose_color_uint (unsigned int value, VerboseColor color)
{
    verbose_color_begin (stdout, color);
    verbose_u (value);
    verbose_color_end (stdout);
}
