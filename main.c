#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <errno.h>

#include "tree.h"

/* General error reporting */
#define LASSERT(args, cond, fmt, ...) \
  do { \
    if (!(cond)) {\
      lval* err = lval_err(fmt, ##__VA_ARGS__); \
      lval_del(args); \
      return err; \
    } \
  } while (0)

/* Type error reporting */
#define LASSERT_TYPE(args, name, num, exp) \
  do { \
    LASSERT(args, args->cell[num]->type == (exp), \
      "Function '%s' passed incorrect type for argument %d. " \
      "Got %s, Expected %s.", \
      name, num, \
      ltype_name(args->cell[num]->type), ltype_name(exp)); \
  } while (0)

/* Argument count error reporting */
#define LASSERT_COUNT(args, name, exp) \
  do { \
    LASSERT(args, (args)->count == (exp), \
      "Function '%s' passed wrong number of arguments. " \
      "Got %d, Expected %d.", \
      name, (args)->count, exp); \
  } while (0)

/* Empty list error reporting */
#define LASSERT_LIST(args, name) \
  do { \
    LASSERT(args, args->cell[0]->count != 0, \
      "Function '%s' passed {}.", \
      name); \
  } while (0)

#ifdef _WIN32

/* Fake Readline on Windows */
char* readline(const char* prompt)
{
  const size_t buffer_len = 2048;
  char* buffer = (char*)malloc(buffer_len);

  /* Output prompt */
  fputs(prompt, stdout);

  /* Read input*/
  char* result = fgets(buffer, buffer_len, stdin);

  /* If read went fine */
  if (result != NULL)
  {
    /* Terminate input string */
    size_t len = strnlen(buffer, buffer_len);
    buffer[len-1] = 0;

    return buffer;
  } else {
    /* Something was wrong */
    free(buffer);
    return NULL;
  }
}

/* Stub */
void add_history(const char* line) 
{
  /* Do nothing */
  (void)line;
}

#undef BUFFER_LEN

#else

#include <readline/readline.h>
#include <readline/history.h>

#endif /* _WIN32 */

const char* copyright =
  "Lisp version 0.0.1 Copyright (C) 2019 Barracuda72\n"
  "This program comes with ABSOLUTELY NO WARRANTY; for details type \"show w\".\n"
  "This is free software, and you are welcome to redistribute it\n"
  "under certain conditions; type \"show c\" for details.\n"
  ;

/*
 * Parser stuff
 */
typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern void free_pool(void);
extern tree* root;
#ifdef DEBUG
extern int yydebug;
#endif

/*
 * Lisp stuff
 */

/* Possible lval types */
typedef enum { LVAL_FUN, LVAL_NUMBER, LVAL_FNUMBER, LVAL_ERROR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR } lval_type_t;

/* Forward declarations */
struct _lval;
struct _lenv;

typedef struct _lval lval;
typedef struct _lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);
void lval_del(lval* v);
lval* lval_copy(lval* v);
lval* lval_err(const char* fmt, ...);
void lval_print(lval* v);
const char* ltype_name(lval_type_t t);

/* Structure that holds value of operation */
typedef struct _lval
{
  lval_type_t type;
  union {
    long num;
    char* err;
    char* sym;
    double fnum;
    struct {
      lbuiltin fun;
      const char* name;
      int builtin;
    };
    struct {
      int count;
      struct _lval** cell;
    };
  };
} lval;

/* Environment structure */
typedef struct _lenv
{
  int count;
  char** syms;
  lval** vals;
} lenv;

/* Create environment */
lenv* lenv_new(void)
{
  lenv* e = (lenv*)malloc(sizeof(lenv));

  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;

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

  return lval_err("Unbound symbol!");
}

/* Put value into environment */
int lenv_put(lenv* e, lval* k, lval* v)
{
  /* First check if this symbol already exists */
  for (int i = 0; i < e->count; i++)
    if (!strcmp(e->syms[i], k->sym))
    {
      lval* o = e->vals[i];
      if ((o->type == LVAL_FUN) && o->builtin)
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
  v->fun = f;
  v->name = name;
  v->builtin = builtin;
  return v;
}

lval* lval_fun(lbuiltin f)
{
  return lval_fun_ex(f, NULL, 0);
}

/* Clear memory occupied by lval */
void lval_del(lval* v)
{
  switch (v->type)
  {
    case LVAL_NUMBER:
    case LVAL_FNUMBER:
    case LVAL_FUN:
      break;

    case LVAL_SYM:
      free(v->sym);
      break;

    case LVAL_ERROR:
      free(v->err);
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
      x->fun = v->fun;
      x->name = v->name;
      x->builtin = v->builtin;
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

    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;

    case LVAL_QEXPR:
      lval_expr_print(v, '{', '}');
      break;

    case LVAL_FUN:
      if (v->builtin)
        fprintf(stdout, "<function '%s'>", v->name);
      else
        fprintf(stdout, "<function>");
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

/* Forward declaration */
lval* lval_eval(lenv* e, lval* v);

/* Builtin operations */
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

lval* lval_join(lval* x, lval* y)
{
  while (y->count > 0)
    x = lval_add(x, lval_pop(y, 0));

  lval_del(y);
  return x;
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

lval* builtin_def(lenv* e, lval* x)
{
  LASSERT_TYPE(x, "def", 0, LVAL_QEXPR);

  lval* syms = x->cell[0];

  for (int i = 0; i < syms->count; i++)
    LASSERT(x, syms->cell[i]->type == LVAL_SYM,
      "Function 'def' can only define symbols but "
      "non-symbol %s was passed as %d argument",
      ltype_name(syms->cell[i]->type), i
      );

  LASSERT(x, syms->count == x->count-1,
    "Function 'def' accepts two lists of matching length but "
    "parameter lengths differ (%d vs %d)",
    syms->count, x->count-1);

  lval* ret = lval_sexpr();

  for (int i = 0; i < syms->count; i++)
    if (lenv_put(e, syms->cell[i], x->cell[i+1]))
    {
      lval_del(ret);
      ret = lval_err("Redefinition of '%s' is forbidden", syms->cell[i]->sym);
      break;
    }

  lval_del(x);

  return ret;
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

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);
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
  lval* result = f->fun(e, v);
  lval_del(f);
  return result;
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

#if 0
void walk_tree(tree* t, int level)
{
  const int step = 4;
  if (t != NULL)
  {
    if (t->type != NODE_CINT && t->type != NODE_IDENTIFIER)
      walk_tree(t->left, level+step);
    for (int i = 0; i < level; i++)
      fputc(' ', stdout);
    fprintf(stdout, "%d\n", t->type);
    if (t->type != NODE_CINT && t->type != NODE_IDENTIFIER)
      walk_tree(t->right, level+step);
  }
}
#endif

int main(int argc, char* argv[])
{
  fputs (copyright, stdout);
  fputs ("Press Ctrl+C to exit prompt\n\n", stdout);

#ifdef DEBUG
  yydebug = 1;
#endif
  lenv* e = lenv_new();
  lenv_add_builtins(e);

  /* Never-ending prompt */
  while (1)
  {
    /* Output prompt and read user input */
    char* input = readline("lisp> ");

    /* Ctrl+D was pressed, terminate session */
    if (input == NULL)
    {
      fputc('\n', stdout);
      break;
    }

    /* Add line to history */
    add_history(input);

    /* Parse */
    YY_BUFFER_STATE buffer = yy_scan_string(input);
    yyparse();
    yy_delete_buffer(buffer);

    /* Free buffer */
    free(input);

    /* Convert input into S-expr */
    //walk_tree(root, 0);
    lval* x = lval_read(root);
    //lval_println(x);
    x = lval_eval(e, x);

    /* Free string pool used by parser */
    free_pool();

    /* Perform calculation */
    lval_println(x);
    lval_del(x);
  }

  lenv_del(e);

  return 0;
}

