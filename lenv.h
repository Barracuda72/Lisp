#ifndef __LENV_H__
#define __LENV_H__
/*
 * Lisp Environment
 */

#include "common.h"

/* Environment structure */
typedef struct _lenv
{
  int count;
  char** syms;
  lval** vals;
  lenv* parent;
} lenv;

/* Create environment */
lenv* lenv_new(void);

/* Delete environment */
void lenv_del(lenv* e);

/* Get value from environment */
lval* lenv_get(lenv* e, lval* k);

/* Put value into environment */
int lenv_put(lenv* e, lval* k, lval* v);

/* Put value in outermost (global) environment */
int lenv_def(lenv* e, lval* k, lval* v);

/* Copy environment */
lenv* lenv_copy(lenv* e);

void lenv_add_builtins(lenv* e);

#endif // __LENV_H__
