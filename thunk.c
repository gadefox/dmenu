/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include "thunk.h"
#include "util.h"


#define THUNK_INIT_SIZE  8


void
thunk_init (Thunk *thunk)
{
    thunk->items = NULL;
    thunk->nelements = 0;
}

byte *
thunk_create (Thunk *thunk, uint init_size, uint element_size)
{
    byte *items;

    if ( init_size == 0 )
        init_size = THUNK_INIT_SIZE;

    items = x_malloc (init_size * element_size);
    if ( items == NULL )
        return NULL;

    thunk->items = items;
    thunk->alloc_size = init_size;
    thunk->element_size = element_size;
    thunk->nelements = 0;

    return items;
}

byte *
thunk_double_size (Thunk *thunk, uint min_size)
{
    byte *new_items;
    uint double_size;

    /* double buffer size */
    double_size = thunk->alloc_size << 1;
    if ( double_size < min_size )  /* min_size can be 0: then the condition is ignored (double_size > 0) */
        double_size = min_size;
    
    new_items = x_realloc (thunk->items, thunk_size_to_bytes (thunk, double_size));
    if ( new_items == NULL )
        /* the buffer thunks->items is still valid */
        return NULL;

    /* update the buffer */
    thunk->items = new_items;
    thunk->alloc_size = double_size;
    
    return new_items;
}

byte *
thunk_alloc_next (Thunk *thunk)
{
    byte *items;

    items = thunk->nelements == thunk->alloc_size ? thunk_double_size (thunk, 0) : thunk->items;
    items += thunk_size_to_bytes (thunk, thunk->nelements);

    thunk->nelements++;
    return items;
}
