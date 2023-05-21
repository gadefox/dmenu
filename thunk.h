/* See LICENSE file for copyright and license details. */

#ifndef _THUNK_H_
#define _THUNK_H_

#include "typedef.h"


#define thunk_size_to_bytes(t, i)  ((t)->element_size * (i))
#define thunk_get_item(t, i)       ((t)->items + thunk_size_to_bytes (t, i))
#define thunk_free(t)              (free ((t)->items))


typedef struct {
    byte *items;        /* elements */
    uint alloc_size;    /* allocated elements */
    uint element_size;  /* sizeof (<Element>) */
    uint nelements;     /* elements used */
} Thunk;


void thunk_init (Thunk *thunk);
byte * thunk_create (Thunk *thunk, uint init_size, uint element_size);
byte * thunk_double_size (Thunk *thunk, uint min_size);
byte * thunk_alloc_next (Thunk *thunk);


#endif  /* _THUNK_H_ */
