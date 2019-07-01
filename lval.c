/*
 * Everything About Lisp Values
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "lenv.h"
#include "lval.h"

/* Create number */
lval* lval_num(long x)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_NUMBER;
  v->num = x;
  return v;
}

/* Create floating-point number */
lval* lval_fnum(double x)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_FNUMBER;
  v->fnum = x;
  return v;
}

/* Create error */
lval* lval_err(const char* fmt, ...)
{
  const int err_len = 512;

  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_ERROR;

  v->err = (char*)malloc(err_len);

  va_list va;
  va_start(va, fmt);
  int n = vsnprintf(v->err, err_len, fmt, va);
  va_end(va);

  v->err = realloc(v->err, n+1);
  
  return v;
}

/* Create symbol */
lval* lval_sym(const char* x)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(x)+1);
  strcpy(v->sym, x);
  return v;
}

/* Create S-expression */
lval* lval_sexpr(void)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Create Q-expression */
lval* lval_qexpr(void)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Create function */
lval* lval_fun_ex(lbuiltin f, const char* name, int builtin)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = f;
  v->name = name;
  v->is_builtin = builtin;
  return v;
}

lval* lval_fun(lbuiltin f)
{
  return lval_fun_ex(f, NULL, 0);
}

/* Create lambda */
lval* lval_lambda(lval* formals, lval* body) 
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = NULL;
  v->env = lenv_new();
  v->formals = formals;
  v->body = body;
  v->is_builtin = 0;
  return v;
}

/* Create string */
lval* lval_str(const char* s) 
{
  lval* v = (lval*)malloc(sizeof(lval));

  v->type = LVAL_STR;
  v->str = (char*)malloc(strlen(s) + 1);
  strcpy(v->str, s);
  
  return v;
}

/* Clear memory occupied by lval */
void lval_del(lval* v)
{
  switch (v->type)
  {
    case LVAL_NUMBER:
    case LVAL_FNUMBER:
      break;

    case LVAL_FUN:
      if (!v->is_builtin)
      {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
      break;

    case LVAL_SYM:
      free(v->sym);
      break;

    case LVAL_ERROR:
      free(v->err);
      break;

    case LVAL_STR:
      free(v->str);
      break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      for (int i = 0; i < v->count; i++)
        lval_del(v->cell[i]);
      free(v->cell);
      break;

    default:
      fprintf(stderr, "Unknown lval type %d\n", v->type);
      break;
  }

  free(v);
}

/* Create a copy of lval */
lval* lval_copy(lval* v)
{
  lval* x = (lval*)malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type)
  {
    case LVAL_FUN:
      x->is_builtin = v->is_builtin;
      if (v->is_builtin) 
      {
        x->builtin = v->builtin;
        x->name = v->name;
      } else {
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;

    case LVAL_NUMBER:
      x->num = v->num;
      break;

    case LVAL_FNUMBER:
      x->fnum = v->fnum;
      break;

    case LVAL_ERROR:
      x->err = (char*)malloc(strlen(v->err)+1);
      strcpy(x->err, v->err);
      break;

    case LVAL_SYM:
      x->sym = (char*)malloc(strlen(v->sym)+1);
      strcpy(x->sym, v->sym);
      break;

    case LVAL_STR:
      x->str = (char*)malloc(strlen(v->str)+1);
      strcpy(x->str, v->str);
      break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = (lval**)malloc(sizeof(lval*)*v->count);
      for (int i = 0; i < v->count; i++)
        x->cell[i] = lval_copy(v->cell[i]);
      break;

    default:
      free(x);
      x = lval_err("Cannot copy unknown type!");
      break;
  }

 return x;
}

lval* lval_read_num(tree* t)
{
  errno = 0;
  long x = strtol((char*)t->value, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

lval* lval_read_fnum(tree* t)
{
  errno = 0;
  double x = strtod((char*)t->value, NULL);
  return errno != ERANGE ? lval_fnum(x) : lval_err("Invalid floating number");
}

lval* lval_read_str(tree* t) 
{
  char* contents = (char*)t->value;
  contents[strlen(contents)-1] = '\0';
  char* unescaped = (char*)malloc(strlen(contents+1)+1);
  strcpy(unescaped, contents+1);
  //TODO: unescape
  //unescaped = mpcf_unescape(unescaped);
  lval* str = lval_str(unescaped);
  free(unescaped);
  return str;
}

lval* lval_add(lval* v, lval* x)
{
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*)*v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read(tree* t)
{
  if (t->type == NODE_CINT) 
    return lval_read_num(t);

  if (t->type == NODE_CFLOAT) 
    return lval_read_fnum(t);

  if (t->type == NODE_CSTRING) 
    return lval_read_str(t);

  if (t->type == NODE_IDENTIFIER) 
    return lval_sym((char*)t->value);

  lval* x = NULL;
  if (t->type == NODE_VAR_DECL) 
    x = lval_sexpr();
  else if (t->type == NODE_CONST_DECL)
    x = lval_qexpr();
  else
    return lval_err("Error parsing expression!");

  tree* list = t->value;
  while (list != NULL)
  {
    x = lval_add(x, lval_read(list->value));
    list = list->next;
  }
  return x;
}

const char* ltype_name(lval_type_t t)
{
  switch (t)
  {
    case LVAL_NUMBER:
      return "Number";

    case LVAL_FNUMBER:
      return "Floating-point number";

    case LVAL_SYM:
      return "Symbol";

    case LVAL_ERROR:
      return "Error";

    case LVAL_STR:
      return "String";

    case LVAL_FUN:
      return "Function";

    case LVAL_SEXPR:
      return "S-Expression";

    case LVAL_QEXPR:
      return "Q-Expression";
  }

  return "Unknown";
}

void lval_expr_print(lval* v, char open, char close)
{
  fputc(open, stdout);

  for (int i = 0; i < v->count; i++)
  {
    lval_print(v->cell[i]);
    
    if (i < v->count-1)
      fputc(' ', stdout);
  }

  fputc(close, stdout);
}

/* Print string */
void lval_print_str(lval* v) 
{
  char* escaped = malloc(strlen(v->str)+1);
  strcpy(escaped, v->str);
  // TODO: escape!
  //escaped = mpcf_escape(escaped);
  fprintf(stdout, "\"%s\"", escaped);
  free(escaped);
}

/* Print lval */
void lval_print(lval* v)
{
  switch (v->type)
  {
    case LVAL_NUMBER:
      fprintf(stdout, "%li", v->num);
      break;

    case LVAL_FNUMBER:
      fprintf(stdout, "%lf", v->fnum);
      break;

    case LVAL_ERROR:
      fprintf(stdout, "Error: %s\n", v->err);
      break;

    case LVAL_SYM:
      fprintf(stdout, "%s", v->sym);
      break;

    case LVAL_STR:
      lval_print_str(v);
      break;

    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;

    case LVAL_QEXPR:
      lval_expr_print(v, '{', '}');
      break;

    case LVAL_FUN:
      if (v->builtin) 
      {
        fprintf(stdout, "<builtin function '%s'>", v->name);
      } else {
        fprintf(stdout, "<function> (\\ "); 
        lval_print(v->formals);
        fputc(' ', stdout); 
        lval_print(v->body); 
        fputc(')', stdout);
      }
      break;

    default:
      fprintf(stdout, "Unknown lval %d\n", v->type);
      break;
  }
}

/* Print lval and newline */
void lval_println(lval* v)
{
  lval_print(v);
  fputc('\n', stdout);
}

lval* lval_pop(lval* v, int i)
{
  assert((v->type == LVAL_SEXPR || v->type == LVAL_QEXPR) && v->count > i);

  /* Take element */
  lval* x = v->cell[i];

  /* Move remaining forward */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*)*(v->count - i - 1));

  /* Decrease count */
  v->count--;

  /* Reallocate memory */
  v->cell = realloc(v->cell, sizeof(lval*)*v->count);

  return x;
}

lval* lval_take(lval* v, int i)
{
  assert(v->type == LVAL_SEXPR);
 
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

int lval_eq(lval* x, lval* y) 
{
  if (x->type != y->type) 
    return 0;

  switch (x->type) 
  {
    case LVAL_NUMBER: 
      return (x->num == y->num);

    case LVAL_FNUMBER: 
      return (x->fnum == y->fnum);

    case LVAL_ERROR: 
      return !strcmp(x->err, y->err);
    case LVAL_SYM: 
      return !strcmp(x->sym, y->sym);
    case LVAL_STR: 
      return !strcmp(x->str, y->str);

    case LVAL_FUN:
      if (x->is_builtin || y->is_builtin)
        return x->builtin == y->builtin;
      else
        return lval_eq(x->formals, y->formals)
          && lval_eq(x->body, y->body);

    case LVAL_QEXPR:
    case LVAL_SEXPR:
      if (x->count != y->count) 
        return 0;
      for (int i = 0; i < x->count; i++)
        if (!lval_eq(x->cell[i], y->cell[i]))
          return 0;
      return 1;
  }

  return 0;
}

lval* lval_join(lval* x, lval* y)
{
  while (y->count > 0)
    x = lval_add(x, lval_pop(y, 0));

  lval_del(y);
  return x;
}

/* Comparison */
int lval_less(lval* a, lval* b)
{
  /* If one of them is float, compare as floats */
  if (a->type == LVAL_NUMBER && b->type == LVAL_FNUMBER)
    return a->num < b->fnum;
  
  if (a->type == LVAL_FNUMBER && b->type == LVAL_NUMBER)
    return a->fnum < b->num;
  
  if (a->type == LVAL_FNUMBER && b->type == LVAL_FNUMBER)
    return a->fnum < b->fnum;
  
  if (a->type == LVAL_NUMBER && a->type == LVAL_NUMBER) 
    return a->num < b->num;

  assert(0);
}

lval* lval_eval_sexpr(lenv* e, lval* v)
{
  assert(v->type == LVAL_SEXPR);

#if 0
  fprintf(stdout, "Evaluating expression ");
  lval_println(v);
#endif

  /* Evaluate children */
  for (int i = 0; i < v->count; i++)
    v->cell[i] = lval_eval(e, v->cell[i]);

  /* Check for errors */
  for (int i = 0; i < v->count; i++)
    if (v->cell[i]->type == LVAL_ERROR)
      return lval_take(v, i);
  
  /* Empty expression */
  if (v->count == 0)
    return v;

  /* Single expression */
  if (v->count == 1)
    return lval_take(v, 0);

  /* Ensure the first element is symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN)
  {
    lval_del(f);
    lval_del(v);
    return lval_err("First element is not a function!");
  }

  /* Compute */
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

lval* lval_call(lenv* e, lval* f, lval* a) 
{
  if (f->is_builtin) 
    return f->builtin(e, a);

  int given = a->count;
  int total = f->formals->count;

  while (a->count) 
  {
    if (f->formals->count == 0) 
    {
      lval_del(a); 
      return lval_err(
        "Function passed too many arguments. "
        "Got %i, Expected %i.", given, total);
    }

    lval* sym = lval_pop(f->formals, 0);

    /* Special Case to deal with '&' */
    if (!strcmp(sym->sym, "&")) 
    {
      /* Ensure '&' is followed by another symbol */
      if (f->formals->count != 1) 
      {
        lval_del(a);
        return lval_err("Function format invalid. "
          "Symbol '&' not followed by single symbol.");
      }

      /* Next formal should be bound to remaining arguments */
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym); 
      lval_del(nsym);
      break;
    } else {
      lval* val = lval_pop(a, 0);
      lenv_put(f->env, sym, val);
      lval_del(sym); 
      lval_del(val);
    }
  }

  lval_del(a);

  if (f->formals->count > 0 &&
    !strcmp(f->formals->cell[0]->sym, "&")) 
  {
    
    if (f->formals->count != 2) 
    {
      return lval_err("Function format invalid. "
        "Symbol '&' not followed by single symbol.");
    }
    
    lval_del(lval_pop(f->formals, 0));
    
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();
    
    lenv_put(f->env, sym, val);
    lval_del(sym); 
    lval_del(val);
  }

  if (f->formals->count == 0) 
  {
    f->env->parent = e;
    return builtin_eval(
      f->env, 
      lval_add(lval_sexpr(), lval_copy(f->body))
    );
  } else {
    return lval_copy(f);
  }
}

lval* lval_eval(lenv* e, lval* v)
{
#if 0
  fprintf(stdout, "Evaluating %d\n", v->type);
#endif

  if (v->type == LVAL_SYM)
  {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(e, v);

  /* All other types remain the same */
  return v;
}

