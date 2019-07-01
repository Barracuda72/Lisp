/*
 * Lisp Environment
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lenv.h"
#include "lval.h"

/* Create environment */
lenv* lenv_new(void)
{
  lenv* e = (lenv*)malloc(sizeof(lenv));

  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  e->parent = NULL;

  return e;
}

/* Delete environment */
void lenv_del(lenv* e)
{
  for (int i = 0; i < e->count; i++)
  {
    lval_del(e->vals[i]);
    free(e->syms[i]);
  }

  free(e->vals);
  free(e->syms);
  free(e);
}

/* Get value from environment */
lval* lenv_get(lenv* e, lval* k)
{
  for (int i = 0; i < e->count; i++)
    if (!strcmp(e->syms[i], k->sym))
      return lval_copy(e->vals[i]);

  if (e->parent)
    return lenv_get(e->parent, k);

  return lval_err("Unbound symbol '%s'!", k->sym);
}

/* Put value into environment */
int lenv_put(lenv* e, lval* k, lval* v)
{
  /* First check if this symbol already exists */
  for (int i = 0; i < e->count; i++)
    if (!strcmp(e->syms[i], k->sym))
    {
      lval* o = e->vals[i];
      if ((o->type == LVAL_FUN) && o->is_builtin)
      {
        /* Forbid built-ins redefinition */
        return 1;
      }
      lval_del(o);
      e->vals[i] = lval_copy(v);
      return 0;
    }

  /* If it isn't, then put it there */
  e->count++;
  e->vals = realloc(e->vals, e->count * sizeof(lval*));
  e->syms = realloc(e->syms, e->count * sizeof(char*));

  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = (char*)malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);

  return 0;
}

/* Put value in outermost (global) environment */
int lenv_def(lenv* e, lval* k, lval* v)
{
  while (e->parent)
    e = e->parent;

  return lenv_put(e, k, v);
}

/* Copy environment */
lenv* lenv_copy(lenv* e)
{
  lenv* n = (lenv*)malloc(sizeof(lenv));

  n->parent = e->parent;
  n->count = e->count;
  n->syms = (char**)malloc(sizeof(char*) * n->count);
  n->vals = (lval**)malloc(sizeof(lval*) * n->count);

  for (int i = 0; i < e->count; i++)
  {
    n->syms[i] = (char*)malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }

  return n;
}

void lenv_add_builtin(lenv* e, const char* name, lbuiltin f)
{
  lval* k = lval_sym(name);
  lval* v = lval_fun_ex(f, name, 1);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e)
{
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "car", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "cdr", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "append", builtin_join);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "init", builtin_init);
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "exit", builtin_exit);
  lenv_add_builtin(e, "env", builtin_env);
  lenv_add_builtin(e, "\\", builtin_lambda);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=",   builtin_put);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);

  /* Comparison Functions */
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_ne);
  lenv_add_builtin(e, ">",  builtin_gt);
  lenv_add_builtin(e, "<",  builtin_lt);
  lenv_add_builtin(e, ">=", builtin_ge);
  lenv_add_builtin(e, "<=", builtin_le);

  /* Logical functions */
  lenv_add_builtin(e, "and", builtin_and);
  lenv_add_builtin(e, "or", builtin_or);
  lenv_add_builtin(e, "xor", builtin_xor);
  lenv_add_builtin(e, "not", builtin_not);
  lenv_add_builtin(e, "&&", builtin_and);
  lenv_add_builtin(e, "||", builtin_or);
  lenv_add_builtin(e, "^", builtin_xor);
  lenv_add_builtin(e, "!", builtin_not);

  /* Misc */
  lenv_add_builtin(e, "load",  builtin_load);
  lenv_add_builtin(e, "error", builtin_error);
  lenv_add_builtin(e, "print", builtin_print);
}

