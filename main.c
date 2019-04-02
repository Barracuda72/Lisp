#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "tree.h"

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
typedef enum { LVAL_NUMBER, LVAL_FNUMBER, LVAL_ERROR, LVAL_SYM, LVAL_SEXPR } lval_type_t;

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
      int count;
      struct _lval** cell;
    };
  };
} lval;

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
lval* lval_err(char* x)
{
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_ERROR;
  v->err = malloc(strlen(x)+1);
  strcpy(v->sym, x);
  return v;
}

/* Create symbol */
lval* lval_sym(char* x)
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

void lval_del(lval* v)
{
  switch (v->type)
  {
    case LVAL_NUMBER:
      break;

    case LVAL_SYM:
      free(v->sym);
      break;

    case LVAL_ERROR:
      free(v->err);
      break;

    case LVAL_SEXPR:
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
  if (t->type == NODE_STMT_LIST) 
    x = lval_sexpr();
  else
    return lval_err("Error parsing expression!");

  tree* list = t;
  while (list != NULL)
  {
    if (list->value != NULL)
      x = lval_add(x, lval_read(list->value));
    else
      x = lval_add(x, lval_sexpr());
    list = list->next;
  }
  return x;
}

void lval_print(lval* v);

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
  assert(v->type == LVAL_SEXPR && v->count > i);

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

lval* builtin_op(lval* a, char* op)
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

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v)
{
  assert(v->type == LVAL_SEXPR);

#if 0
  fprintf(stdout, "Evaluating expression ");
  lval_println(v);
#endif

  /* Evaluate children */
  for (int i = 0; i < v->count; i++)
    v->cell[i] = lval_eval(v->cell[i]);

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
  if (f->type != LVAL_SYM)
  {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression doesn't start with the symbol!");
  }

  /* Compute */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v)
{
#if 0
  fprintf(stdout, "Evaluating %d\n", v->type);
#endif

  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(v);

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
    x = lval_eval(x);

    /* Free string pool used by parser */
    free_pool();

    /* Perform calculation */
    lval_println(x);
    lval_del(x);
  }

  return 0;
}

