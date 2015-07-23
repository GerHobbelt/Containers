/* main.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base/memento/memento.h"

#include "base/result.h"
#include "base/types.h"

#include "test.h"

result_t queuetest(void);

int main(int argc, char *argv[])
{
  int viz = 1;

  // Memento_setParanoia(1);

  if (argc > 1)
    while (++argv, --argc)
    {
      if (strcmp(argv[0], "-viz") == 0)
        viz = 1;
      else if (strcmp(argv[0], "-noviz") == 0)
        viz = 0;
      else
      {
        fprintf(stderr, "Unrecognised option: '%s'\n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }

  (void) queuetest();

  test_container(viz);

  return EXIT_SUCCESS;
}
