/* See LICENSE file for copyright and license details. */

#ifndef _ARGS_H_
#define _ARGS_H_

#include "drw.h"


typedef enum {
    FlagVersion  = (1 << 0),
    FlagHelp     = (1 << 1),
    FlagFast     = (1 << 2),
    FlagAlignTop = (1 << 3)
} Flags;


extern Flags a_flags;
extern int a_rows;
extern int a_mon;
extern const char *a_prompt;
extern char *a_font;
extern const char *a_color [ColorMax];
extern char *a_items;
extern Window a_winid;
extern uint a_prompt_w;


extern const char version_arg[];
extern const char help_arg[];
extern const char fast_arg[];
extern const char casei_arg[];
extern const char rows_arg[];
extern const char mon_arg[];
extern const char prompt_arg[];
extern const char font_arg[];
extern const char nfg_arg[];
extern const char nbg_arg[];
extern const char sfg_arg[];
extern const char sbg_arg[];
extern const char winid_arg[];
extern const char items_arg[];


/*
 * Args
 */

typedef struct {
    int count;
    char **v;
    char *arg;
    char *name;
    char *value;
} Args;

typedef enum {
    ArgsEnd,
    ArgsDouble,
    ArgsSingle,
    ArgsWord
} ArgsStatus;

void args_copy_remaining (Args *copy, Args *args);
ArgsStatus args_fetch_next (Args *args, const char *argname, int no_parse );
int args_parse (Args *args);


#endif  /* _ARGS_H_ */
