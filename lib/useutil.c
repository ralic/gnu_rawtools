/*** useutil.c --- Useful utility functions  -*- C -*- */

/*** Copyright (C) 2006, 2007 Ivan Shmakov */

/** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful, but
 ** WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 ** 02110-1301 USA
 */

/*** Code: */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usemacro.h"
#include "useutil.h"

int
ensure_enough_space (void **ptr, size_t *allocptr,
                     size_t size, size_t wanted)
{
  const size_t alloc = (*ptr != 0) ? *allocptr : 0;
  if (wanted <= alloc) {
    /* . */
    return 0;
  }

  {
    /* FIXME: magic formulae */
    const size_t
      sz = MAX (MAX (3, (alloc << 1) + 1), wanted);
    void *new;
    if ((new = realloc (*ptr, size * sz)) == 0) {
      /* . */
      return -1;
    }
    *ptr = new;
    *allocptr = sz;
  }

  /* . */
  return 0;
}

/*** Vectors of Strings */

int
strings_append (struct strings *vec, const char **s, size_t count)
{
  if (count == 0) {
    /* . */
    return 0;
  }
  if (ensure_enough_space ((void **)&(vec->s), &(vec->alloc),
                           sizeof (*vec->s), count + vec->size)
      != 0) {
    /* . */
    return -1;
  }
  COPY_ARY (vec->s + vec->size, s, count);
  vec->size += count;

  /* . */
  return 0;
}

void
strings_clear (struct strings *vec)
{
  if (vec->s != 0) free (vec->s);
  vec->size = vec->alloc = 0;
}

/*** Usual file I/O */

FILE *
open_file (const char *arg, int input_p)
{
  FILE *fp;

  if (arg == 0) {
    errno = EINVAL;
    /* . */
    return 0;
  } else if (strcmp ("-", arg) == 0) {
    /* . */
    return input_p ? stdin : stdout;
  } else if ((fp = fopen (arg, input_p ? "r" : "w")) == 0) {
    /* . */
    return 0;
  }

  /* . */
  return fp;
}

void
close_file (FILE *fp)
{
  if (fp == stdin || fp == stdout) {
    /* . */
    return;
  }

  fclose (fp);
  /* . */
}

/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "british" */
/** mode: outline-minor */
/** outline-regexp: "/[*][*][*]" */
/** End: */
/** LocalWords:   */
/*** useutil.c ends here */
