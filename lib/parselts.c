/*** parselts.c --- Parsing arrays and their elements  -*- C -*- */

/*** Copyright (C) 2007 Ivan Shmakov */

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
#include <ctype.h>              /* for isspace () */
#include <errno.h>
#include <stdlib.h>
#include <string.h>             /* for memcpy () */

#include "parselts.h"
#include "usemacro.h"           /* for COPY_ARY () */
#include "useutil.h"            /* for ensure_enough_space () */

/* a type representing a wrapper around an ``integer'' `strto*'
   function */
typedef int (*strto_i_fn) (void *buf, const char *string,
                           char **tailptr, int base);

/* a type representing a wrapper around a ``float'' `strto*' function */
typedef int (*strto_f_fn) (void *buf, const char *string,
                           char **tailptr);

/*** Utility */

/* FIXME: duplicates p_arg_skipspace () */
static const char *
skip_space (const char *s)
{
  if (s == 0) return 0;         /* . */
  while (isspace (*s)) s++;
  /* . */
  return s;
}

static const char *
skip_space_after_a_number (const char *s,
                           const struct parse_elt_number_param *npar)
{
  /* . */
  return (s == 0 || ! npar->allow_whitespace_after_p ? s
          : skip_space (s));
}

/*** Wrappers around `strto*' functions */

#define STRTO_I_WRAP(fn, type, strtox) \
    static int fn (type *buf, const char *string, \
                   char **tailptr, int base) \
    { \
      type v = strtox (string, tailptr, base); \
      if (buf != 0) *buf = v; \
      /* . */ \
      return v == -1 ? -1 : 0; \
    }

#define STRTO_F_WRAP(fn, type, strtox) \
    static int fn (type *buf, const char *string, \
                   char **tailptr) \
    { \
      type v = strtox (string, tailptr); \
      if (buf != 0) *buf = v; \
      /* . */ \
      return 0; \
    }

STRTO_I_WRAP (strtol_wrap,   long, strtol) ;
STRTO_I_WRAP (strtoll_wrap,  long long, strtoll) ;
STRTO_I_WRAP (strtoul_wrap,  unsigned long, strtoul) ;
STRTO_I_WRAP (strtoull_wrap, unsigned long long, strtoull) ;

STRTO_F_WRAP (strtof_wrap,   float, strtof) ;
STRTO_F_WRAP (strtod_wrap,   double, strtod) ;
STRTO_F_WRAP (strtold_wrap,  long double, strtold) ;

/*** Parsing single elements */

struct parse_elt_strto_i_param {
  const struct parse_elt_integer_param *integer_param_ptr;
  strto_i_fn wrapper;
  size_t elt_sz;
};

struct parse_elt_strto_f_param {
  const struct parse_elt_integer_param *number_param_ptr;
  strto_f_fn wrapper;
  size_t elt_sz;
};

static int
parse_elt_strto_i (const char *string, void *buf, char **tailptr,
                   struct parse_elt_strto_i_param *param)
{
  const struct parse_elt_number_param *const
    npar = &(param->integer_param_ptr->number_param);
  const int base = param->integer_param_ptr->base;
  strto_i_fn strto_i = param->wrapper;
  char *t;
  char vb[param->elt_sz];
  int saved_errno = errno;

  errno = 0;
  if (strto_i (vb, string, &t, base),
      t == string) {
    if (tailptr != 0) *tailptr = string;
    errno = EINVAL;
    /* . */
    return -1;
  }
  if (errno != 0
      && (! npar->allow_out_of_range_p || errno != ERANGE)) {
    if (tailptr != 0) *tailptr = string;
    /* . */
    return -1;
  }
  errno = saved_errno;

  if (tailptr != 0) *tailptr = skip_space_after_a_number (t, npar);
  if (buf != 0) COPY_ARY (buf, vb, param->elt_sz);

  /* . */
  return 0;
}

static int
parse_elt_strto_f (const char *string, void *buf, char **tailptr,
                   struct parse_elt_strto_f_param *param)
{
  const struct parse_elt_number_param *const
    npar = param->number_param_ptr;
  strto_f_fn strto_f = param->wrapper;
  char *t;
  char vb[param->elt_sz];
  int saved_errno = errno;

  errno = 0;
  if (strto_f (vb, string, &t),
      t == string) {
    if (tailptr != 0) *tailptr = string;
    errno = EINVAL;
    /* . */
    return -1;
  }
  if (errno != 0
      && (! npar->allow_out_of_range_p || errno != ERANGE)) {
    if (tailptr != 0) *tailptr = string;
    /* . */
    return -1;
  }
  errno = saved_errno;

  if (tailptr != 0) *tailptr = skip_space_after_a_number (t, npar);
  if (buf != 0) COPY_ARY (buf, vb, param->elt_sz);

  /* . */
  return 0;
}

#define PARSE_ELT_STRTO_I_WRAP(fn, type, wrapfn) \
    int \
    fn (const char *string, type *buf, char **tailptr, \
        struct parse_elt_integer_param *param) \
    { \
      struct parse_elt_strto_i_param p; \
      p.integer_param_ptr = param; \
      p.wrapper = (strto_i_fn)wrapfn; \
      p.elt_sz = sizeof (type); \
      /* . */ \
      return parse_elt_strto_i (string, buf, tailptr, &p); \
    }

#define PARSE_ELT_STRTO_F_WRAP(fn, type, wrapfn) \
    int \
    fn (const char *string, type *buf, char **tailptr, \
        struct parse_elt_number_param *param) \
    { \
      struct parse_elt_strto_f_param p; \
      p.number_param_ptr = param; \
      p.wrapper = (strto_f_fn)wrapfn; \
      p.elt_sz = sizeof (type); \
      /* . */ \
      return parse_elt_strto_f (string, buf, tailptr, &p); \
    }

PARSE_ELT_STRTO_I_WRAP (parse_elt_long, long,
                        strtol_wrap) ;
PARSE_ELT_STRTO_I_WRAP (parse_elt_ulong, unsigned long,
                        strtoul_wrap) ;
PARSE_ELT_STRTO_I_WRAP (parse_elt_llong, long long,
                        strtoll_wrap) ;
PARSE_ELT_STRTO_I_WRAP (parse_elt_ullong, unsigned long long,
                        strtoull_wrap) ;

PARSE_ELT_STRTO_F_WRAP (parse_elt_float, float, strtof_wrap) ;
PARSE_ELT_STRTO_F_WRAP (parse_elt_double, double, strtod_wrap) ;
PARSE_ELT_STRTO_F_WRAP (parse_elt_ldouble, long double, strtold_wrap) ;

/*** Loading arrays from delimited strings */

int
parse_elts_delim (const char *string, void **buf, size_t elt_sz,
                  size_t *allocptr, size_t *sizeptr,
                  int delim, char **tailptr,
                  parse_elt_fn parse_elt, void *param)
{
  int count;
  const char *p;

  for (p = string, count = 0; *p != '\0'; count++) {
    char *t;
    int rv;
    char b1[elt_sz];

    /* Check for an element at the pointer (`p') */
    if ((rv = parse_elt (p, b1, &t, param)),
        t == p) {
      if (tailptr != 0) *tailptr = p;
      /* . */
      return count;
    }

    /* Store the value, enlarging the array as necessary */
    if (ensure_enough_space ((void **)buf, allocptr,
                             elt_sz, (*sizeptr) + 1)
        != 0) {
      /* . */
      return -1;
    }
    COPY_ARY ((char *)(*buf + *sizeptr * elt_sz), b1, elt_sz);
    (*sizeptr)++;

    /* Check for the `delim' at the tail (`t') */
    if (*t != '\0' && *t != delim) {
      if (tailptr != 0) *tailptr = t;
      /* . */
      return count;
    }
    p = (*t == delim ? t + 1 : t);
  }

  if (tailptr != 0) *tailptr = p;
  /* . */
  return count;
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
/*** parselts.c ends here */
