#ifndef __BUILTINS_H__
#define __BUILTINS_H__
/*
 * Built-in interpreter functions
 */

#include "common.h"

typedef lval* (*lbuiltin)(lenv*, lval*);

lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_exit(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_env(lenv* e, lval* a);
lval* builtin_add(lenv* e, lval* x);
lval* builtin_sub(lenv* e, lval* x);
lval* builtin_mul(lenv* e, lval* x);
lval* builtin_div(lenv* e, lval* x);
lval* builtin_mod(lenv* e, lval* x);
lval* builtin_lambda(lenv* e, lval* a);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_ge(lenv* e, lval* a);
lval* builtin_le(lenv* e, lval* a);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
lval* builtin_if(lenv* e, lval* a);
lval* builtin_and(lenv* e, lval* a);
lval* builtin_or(lenv* e, lval* a);
lval* builtin_xor(lenv* e, lval* a);
lval* builtin_not(lenv* e, lval* a);
lval* builtin_load(lenv* e, lval* a);
lval* builtin_print(lenv* e, lval* a);
lval* builtin_error(lenv* e, lval* a);

#endif // __BUILTINS_H__
