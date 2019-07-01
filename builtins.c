/*
 * builtins.c
 *
 * Built-in interpreter functions
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lenv.h"
#include "lval.h"
#include "builtins.h"
#include "lassert.h"
#include "parser.h"

lval* builtin_head(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "head", 1);
  LASSERT_TYPE(a, "head", 0, LVAL_QEXPR);
  LASSERT_LIST(a, "head");

  lval* v = lval_take(a, 0);
  while (v->count > 1) lval_del(lval_pop(v, 1));
  return v;
}

lval* builtin_tail(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "tail", 1);
  LASSERT_TYPE(a, "tail", 0, LVAL_QEXPR);
  LASSERT_LIST(a, "tail");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a)
{
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "eval", 1);
  LASSERT_TYPE(a, "eval", 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

/* Join two lists together */
lval* builtin_join(lenv* e, lval* a)
{
  for (int i = 0; i < a->count; i++)
    LASSERT_TYPE(a, "join", i, LVAL_QEXPR);

  lval* x = lval_pop(a, 0);

  while (a->count > 0)
    x = lval_join(x, lval_pop(a, 0));

  lval_del(a);

  return x;
}

/* Remove last element of list */
lval* builtin_init(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "init", 1);
  LASSERT_TYPE(a, "init", 0, LVAL_QEXPR);
  LASSERT_LIST(a, "init");

  lval* v = lval_take(a, 0);
  lval* x = lval_qexpr();

  int n = v->count - 1;

  for (int i = 0; i < n; i++)
    x = lval_add(x, lval_pop(v, 0));

  lval_del(v);

  return x;
}

/* Calculate length of list */
lval* builtin_len(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "len", 1);
  LASSERT_TYPE(a, "len", 0, LVAL_QEXPR);

  int len = a->cell[0]->count;
  lval_del(a);
  return lval_num(len);
}

/* Exit command prompt */
lval* builtin_exit(lenv* e, lval* a)
{
  /* TODO */
  return lval_sexpr();
}

/* Prepend element to list */
lval* builtin_cons(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "cons", 2);
  LASSERT_TYPE(a, "cons", 1, LVAL_QEXPR);

  lval* x = lval_qexpr();

  lval_add(x, lval_pop(a, 0));
  lval* v = lval_take(a, 0);

  x = lval_join(x, v);

  return x;
}

lval* builtin_env(lenv* e, lval* a)
{
  lval* v = lval_qexpr();

  for (int i = 0; i < e->count; i++)
  {
    lval* p = lval_qexpr();
    lval_add(p, lval_sym(e->syms[i]));
    lval_add(p, lval_copy(e->vals[i]));
    lval_add(v, p);
  }

  return v;
}

lval* builtin_op(lenv* e, lval* a, const char* op)
{
  assert(a->type == LVAL_SEXPR);

  /* Ensure operands are numbers */
  for (int i = 0; i < a->count; i++)
    if (a->cell[i]->type != LVAL_NUMBER && a->cell[i]->type != LVAL_FNUMBER)
    {
      lval_del(a);
      return lval_err("Cannot operate on non-numbers!");
    }

  /* Take first element */
  lval* x = lval_pop(a, 0);

  /* Unary negation */
  if (!strcmp(op, "-") && (a->count == 0))
  {
    x->num = -x->num;
  }

  while (a->count > 0)
  {
    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (!strcmp(op, "+"))
      x->num += y->num;

    if (!strcmp(op, "-"))
      x->num -= y->num;

    if (!strcmp(op, "*"))
      x->num *= y->num;

    if (!strcmp(op, "/"))
    {
      if (y->num == 0)
      {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division By Zero!");
        break;
      }
      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* builtin_add(lenv* e, lval* x)
{
  return builtin_op(e, x, "+");
}

lval* builtin_sub(lenv* e, lval* x)
{
  return builtin_op(e, x, "-");
}

lval* builtin_mul(lenv* e, lval* x)
{
  return builtin_op(e, x, "*");
}

lval* builtin_div(lenv* e, lval* x)
{
  return builtin_op(e, x, "/");
}

lval* builtin_mod(lenv* e, lval* x)
{
  return builtin_op(e, x, "%");
}

/* Define new variable */
lval* builtin_var(lenv* e, lval* x, const char* func)
{
  LASSERT_TYPE(x, func, 0, LVAL_QEXPR);

  lval* syms = x->cell[0];

  for (int i = 0; i < syms->count; i++)
    LASSERT(x, syms->cell[i]->type == LVAL_SYM,
      "Function '%s' can only define symbols but "
      "non-symbol %s was passed as %d argument",
      func,
      ltype_name(syms->cell[i]->type), i
      );

  LASSERT(x, syms->count == x->count-1,
    "Function '%s' accepts two lists of matching length but "
    "parameter lengths differ (%d vs %d)",
    func, syms->count, x->count-1);

  lval* ret = lval_sexpr();

  int (*putter)(lenv*, lval*, lval*) = NULL;

  if (!strcmp(func, "def"))
    putter = lenv_def;

  if (!strcmp(func, "="))
    putter = lenv_put;

  assert(putter != NULL);

  for (int i = 0; i < syms->count; i++)

    if (putter(e, syms->cell[i], x->cell[i+1]))
    {
      lval_del(ret);
      ret = lval_err("Redefinition of '%s' is forbidden", syms->cell[i]->sym);
      break;
    }

  lval_del(x);

  return ret;
}

/* Lambda function */
lval* builtin_lambda(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "\\", 2);
  LASSERT_TYPE(a, "\\", 0, LVAL_QEXPR);
  LASSERT_TYPE(a, "\\", 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, a->cell[0]->cell[i]->type == LVAL_SYM,
      "Cannot define non-symbol. Got %s, Expected %s.",
      ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM)
    );
  }

  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

/* Definition in global environment */
lval* builtin_def(lenv* e, lval* a)
{
  return builtin_var(e, a, "def");
}

/* Definition in local environment */
lval* builtin_put(lenv* e, lval* a)
{
  return builtin_var(e, a, "=");
}

lval* builtin_ord(lenv* e, lval* a, char* op)
{
  LASSERT_COUNT(a, op, 2);
  LASSERT_TYPE(a, op, 0, LVAL_NUMBER);
  LASSERT_TYPE(a, op, 1, LVAL_NUMBER);

  int result = -1;
  lval* x = a->cell[0];
  lval* y = a->cell[1];

  if (!strcmp(op, ">"))
    result = lval_less(y, x);
  if (strcmp(op, "<")  == 0)
    result = lval_less(x, y);
  if (strcmp(op, ">=") == 0)
    result = !lval_less(x, y);
  if (strcmp(op, "<=") == 0)
    result = !lval_less(y, x);

  lval_del(a);

  assert(result >= 0);

  return lval_num(result);
}

lval* builtin_gt(lenv* e, lval* a)
{
  return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a)
{
  return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a)
{
  return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a)
{
  return builtin_ord(e, a, "<=");
}

lval* builtin_cmp(lenv* e, lval* a, char* op)
{
  LASSERT_COUNT(a, op, 2);

  int r = -1;

  if (!strcmp(op, "=="))
    r =  lval_eq(a->cell[0], a->cell[1]);

  if (!strcmp(op, "!="))
    r = !lval_eq(a->cell[0], a->cell[1]);

  assert(r >= 0);

  lval_del(a);
  return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a)
{
  return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a)
{
  return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "if", 3);
  LASSERT_TYPE(a, "if", 0, LVAL_NUMBER);
  LASSERT_TYPE(a, "if", 1, LVAL_QEXPR);
  LASSERT_TYPE(a, "if", 2, LVAL_QEXPR);

  lval* x;
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  if (a->cell[0]->num)
    x = lval_eval(e, lval_pop(a, 1));
  else
    x = lval_eval(e, lval_pop(a, 2));

  lval_del(a);
  return x;
}

lval* builtin_and(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "and", 2);
  LASSERT_TYPE(a, "and", 0, LVAL_NUMBER);
  LASSERT_TYPE(a, "and", 1, LVAL_NUMBER);

  int r = a->cell[0]->num && a->cell[1]->num;

  lval_del(a);
  return lval_num(r);
}

lval* builtin_or(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "or", 2);
  LASSERT_TYPE(a, "or", 0, LVAL_NUMBER);
  LASSERT_TYPE(a, "or", 1, LVAL_NUMBER);

  int r = a->cell[0]->num || a->cell[1]->num;

  lval_del(a);
  return lval_num(r);
}

lval* builtin_xor(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "xor", 2);
  LASSERT_TYPE(a, "xor", 0, LVAL_NUMBER);
  LASSERT_TYPE(a, "xor", 1, LVAL_NUMBER);

  int r = a->cell[0]->num ^ a->cell[1]->num;

  lval_del(a);
  return lval_num(r);
}

lval* builtin_not(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "not", 1);
  LASSERT_TYPE(a, "not", 0, LVAL_NUMBER);

  int r = !(a->cell[0]->num);

  lval_del(a);
  return lval_num(r);
}

/* Script loading */
lval* builtin_load(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "load", 1);
  LASSERT_TYPE(a, "load", 0, LVAL_STR);

  /* Parse File given by string name */
  char* input = NULL;
  tree* root = NULL;
  FILE* f = fopen(a->cell[0]->str, "r");
  if (f != NULL)
  {
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    input = (char*)malloc(size+1);

    fread(input, size, 1, f);
    fclose(f);

    input[size] = 0;

    root = parse_and_free(input);
  }

  // TODO: better error handling

  if (f != NULL)
  {
    /* Read contents */
    lval* expr = lval_read(root);

    /* Free string pool used by parser */
    parser_free_pool();

    /* Evaluate each Expression */
    while (expr->count)
    {
      lval* x = lval_eval(e, lval_pop(expr, 0));
      /* If Evaluation leads to error print it */
      if (x->type == LVAL_ERROR)
        lval_println(x);
      lval_del(x);
    }

    /* Delete expressions and arguments */
    lval_del(expr);
    lval_del(a);

    /* Return empty list */
    return lval_sexpr();
  } else {
    /* Create new error message using it */
    lval* err = lval_err("Could not load Library %s", a->cell[0]->str);

    lval_del(a);

    /* Cleanup and return error */
    return err;
  }
}

/* Print */
lval* builtin_print(lenv* e, lval* a)
{
  /* Print each argument followed by a space */
  for (int i = 0; i < a->count; i++)
  {
    lval_print(a->cell[i]);
    fputc(' ', stdout);
  }

  /* Print a newline and delete arguments */
  fputc('\n', stdout);
  lval_del(a);

  return lval_sexpr();
}

/* Error generation */
lval* builtin_error(lenv* e, lval* a)
{
  LASSERT_COUNT(a, "error", 1);
  LASSERT_TYPE(a, "error", 0, LVAL_STR);

  /* Construct Error from first argument */
  lval* err = lval_err(a->cell[0]->str);

  /* Delete arguments and return */
  lval_del(a);
  return err;
}

