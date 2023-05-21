/* See LICENSE file for copyright and license details. */

#ifndef _INPUT_H_
#define _INPUT_H_

#include <X11/Xlib.h>
#include "thunk.h"


#define input_begin  ((char *) input.t.items)
#define input_end    (input_begin + strlen (input_begin))

#define input_tok_v  ((char **) input_tok.items)


typedef struct {
    Thunk t;
    uint w;
    char *cursor;
} Input;


extern const char input_delim[];

extern Thunk input_tok;
extern Input input;


int input_init (void);
void input_free (void);

char * input_update_tokens (void);
int input_check_tokens (char *value);

int input_insert (const char *str, int n);
int input_set (const char *str, int n);

int input_next_rune (void);
int input_prev_rune (void);
char * input_next_rune_s (char *s);
char * input_prev_rune_s (char *s);
void input_move_next_word_edge (void);
void input_move_prev_word_edge (void);

int input_delete_word (void);
int input_key_press (KeySym ksym, uint state, char *buf, uint len);


#endif  /* _INPUT_H_ */
