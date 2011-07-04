/* orderedarray-walk.c -- associative array implemented as ordered array */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memento.h"

#include "types.h"
#include "string.h"
#include "string-kv.h"

#include "orderedarray.h"

#include "orderedarray-impl.h"

/* ----------------------------------------------------------------------- */

error orderedarray_walk(const orderedarray_t       *t,
                        orderedarray_walk_callback *cb,
                        void                       *opaque)
{
  error                 err;
  orderedarray__node_t *n;

  if (t == NULL)
    return error_OK;

  /* don't pre-calculate the end position as it needs to be evaluated on
   * every step: the callback is permitted to delete the current element */

  for (n = t->array; n < t->array + t->nelems; n++)
  {
    err = cb(n->item.key, n->item.value, opaque);
    if (err)
      return err;
  }

  return error_OK;
}

/* ----------------------------------------------------------------------- */
