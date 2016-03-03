/* --------------------------------------------------------------------------
 *    Name: show-viz.c
 * Purpose: Associative array implemented as a PATRICIA tree
 * ----------------------------------------------------------------------- */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/result.h"
#include "base/types.h"
#include "base/utils.h"

#include "datastruct/patricia.h"

#include "impl.h"

typedef struct patricia__show_viz_args
{
  patricia_show_key     *key;
  patricia_show_destroy *key_destroy;
  patricia_show_value   *value;
  patricia_show_destroy *value_destroy;
  FILE                  *f;

  int                    rank;
}
patricia__show_viz_args_t;

/* rank the nodes */
static result_t patricia__node_show_viz_rank(patricia__node_t *n,
                                             int               level,
                                             void             *opaque)
{
  patricia__show_viz_args_t *args = opaque;

  NOT_USED(level);

  if (n->bit == args->rank)
    (void) fprintf(args->f, "\t\t\"%p\"\n", (void *) n);

  return result_OK;
}

/* link the nodes */
static result_t patricia__node_show_viz_link(patricia__node_t *n,
                                             int               level,
                                             void             *opaque)
{
  patricia__show_viz_args_t *args = opaque;

  NOT_USED(level);

  {
    static const char *labels[2][2][3] =
    {
      {
        { "sw", "nw", "rnormal"  },
        { "sw", "n",  "ronormal" },
      },
      {
        { "se", "ne", "lnormal"  },
        { "se", "n",  "lonormal" },
      }
    };

    const char *key;
    const char *value;
    int         i;

    key   = args->key   && n->item.key   ? args->key(n->item.key)     : NULL;
    value = args->value && n->item.value ? args->value(n->item.value) : NULL;

    (void) fprintf(args->f, "\t\"%p\" [shape=record, label=\"{bit=%d|{%s|%s}}\"];\n",
                   (void *) n,
                   n->bit,
                   key   ? key   : "(null)",
                   value ? value : "(null)");

    for (i = 0; i < 2; i++)
    {
      int downward;

      if (n->child[i] == NULL)
        continue;

      downward = n->child[i]->bit > n->bit;

      (void) fprintf(args->f, "\t\"%p\":%s -> \"%p\":%s [arrowhead=%s];\n",
                     (void *) n,
                     labels[i][downward][0],
                     (void *) n->child[i],
                     labels[i][downward][1],
                     labels[i][downward][2]);
    }

    if (args->key_destroy   && key)   args->key_destroy((char *) key);
    if (args->value_destroy && value) args->value_destroy((char *) value);
  }

  return result_OK;
}

/* ----------------------------------------------------------------------- */

static void patricia__max_rank_r(const patricia__node_t *n, int *depth)
{
  int i;

  if (n->bit > *depth)
    *depth = n->bit;

  for (i = 0; i < 2; i++)
    if (n->child[i] && n->child[i]->bit > n->bit)
      patricia__max_rank_r(n->child[i], depth);
}

static int patricia__max_rank(const patricia_t *t)
{
  int depth;

  depth = -1;

  patricia__max_rank_r(t->root, &depth);

  return depth;
}

/* ----------------------------------------------------------------------- */

static int patricia__count_for_rank_r(const patricia__node_t *n,
                                      int                     rank,
                                      int                     count)
{
  int i;

  for (i = 0; i < 2; i++)
    if (n->child[i] && n->child[i]->bit > n->bit)
      count = patricia__count_for_rank_r(n->child[i], rank, count);

  return count + (n->bit == rank);
}

static int patricia__count_for_rank(const patricia_t *t, int rank)
{
  return patricia__count_for_rank_r(t->root, rank, 0);
}

/* ----------------------------------------------------------------------- */

result_t patricia_show_viz(const patricia_t      *t,
                           patricia_show_key     *key,
                           patricia_show_destroy *key_destroy,
                           patricia_show_value   *value,
                           patricia_show_destroy *value_destroy,
                           FILE                  *f)
{
  result_t                  err;
  patricia__show_viz_args_t args;
  int                       maxrank;
  int                       i;
  int                       counts[200]; // up to 200 levels (common prefix test hits)

  args.key           = key;
  args.key_destroy   = key_destroy;
  args.value         = value;
  args.value_destroy = value_destroy;
  args.f             = f;

  (void) fprintf(f, "digraph \"patricia\"\n");
  (void) fprintf(f, "{\n");
  (void) fprintf(f, "\tnode [shape = circle];\n");

  /* breadthwalk doesn't make sense for patricia trie - we need to rank the nodes by their critical bit */

  maxrank = patricia__max_rank(t);
  (void) fprintf(f, "\t/* maxrank=%d */\n", maxrank);

  for (i = 0; i <= maxrank; i++) /* note <= */
  {
    counts[i] = patricia__count_for_rank(t, i);
    (void) fprintf(f, "\t/* rank %d has %d nodes */\n", i, counts[i]);
  }

  /* draw a depth chart on the left hand side to force nodes to line up*/

  (void) fprintf(f, "{\n");
  (void) fprintf(f, "\tnode [shape=plaintext, fontsize=16];\n");
  fprintf(f, "\t-1"); /* we always have a node at this level */
  for (i = 0; i <= maxrank; i++)
    if (counts[i])
      fprintf(f, " -> %d", i);
  (void) fprintf(f, ";\n");
  (void) fprintf(f, "\t}\n");

  /* group nodes of the same critbit */

  for (i = -1; i <= maxrank; i++)
  {
    /* there's always a node at level -1 */
    if (i >= 0 && counts[i] == 0)
      continue;

    (void) fprintf(f, "\t{ rank=same; %d /* level %d */\n", i, i);

    args.rank = i;

    err = patricia__walk_internal((patricia_t *) t,
                                  patricia_WALK_IN_ORDER | patricia_WALK_BRANCHES,
                                  patricia__node_show_viz_rank,
                                  &args);
    if (err)
      return err;

    (void) fprintf(f, "\t}\n");
  }

  err = patricia__walk_internal((patricia_t *) t,
                                patricia_WALK_IN_ORDER | patricia_WALK_BRANCHES,
                                patricia__node_show_viz_link,
                                &args);
  if (err)
    return err;

  (void) fprintf(f, "}\n");

  return result_OK;
}

