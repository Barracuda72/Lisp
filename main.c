#include <stdio.h>
#include <stdlib.h>

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
extern tree* root;
#ifdef DEBUG
extern int yydebug;
#endif

/*
 * Lisp stuff
 */
/* Possible error types */
typedef enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM } lval_error_t;

/* Possible lval types */
typedef enum { LVAL_NUMBER, LVAL_ERROR } lval_type_t;

/* Structure that holds value of operation */
typedef struct
{
  lval_type_t type;
  long num;
  lval_error_t err;
} lval;

/* Create number */
lval lval_num(long x)
{
  lval v;
  v.type = LVAL_NUMBER;
  v.num = x;
  return v;
}

/* Create error */
lval lval_err(lval_error_t x)
{
  lval v;
  v.type = LVAL_ERROR;
  v.err = x;
  return v;
}

/* Print lval */
void lval_print(lval v)
{
  switch (v.type)
  {
    case LVAL_NUMBER:
      fprintf(stdout, "%li", v.num);
      break;

    case LVAL_ERROR:
      fputs("Error: ", stdout);
      switch (v.err)
      {
        case LERR_DIV_ZERO:
          fputs("Division by zero\n", stdout);
          break;

        case LERR_BAD_OP:
          fputs("Bad operator\n", stdout);
          break;

        case LERR_BAD_NUM:
          fputs("Bad number\n", stdout);
          break;

        default:
          fprintf(stdout, "Unknown error %d\n", v.err);
          break;
      }
      break;

    default:
      fprintf(stdout, "Unknown lval %d\n", v.type);
      break;
  }
}

/* Print lval and newline */
void lval_println(lval v)
{
  lval_print(v);
  fputc('\n', stdout);
}

lval eval_op(lval x, char* op, lval y)
{
  /* If either is ERROR, return it */
  if (x.type == LVAL_ERROR) return x;
  if (y.type == LVAL_ERROR) return y;

  /* Else perform arithmetic operation */
  if (!strncmp(op, "+", 1)) return lval_num(x.num + y.num);
  if (!strncmp(op, "-", 1)) return lval_num(x.num - y.num);
  if (!strncmp(op, "*", 1)) return lval_num(x.num * y.num);
  if (!strncmp(op, "/", 1)) 
  {
    if (y.num == 0)
      return lval_err(LERR_DIV_ZERO);
    else
      return lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(tree* t)
{
  printf("Tree %lx, ", t);
  printf("type %lx, ", t->type);
  printf("left %lx, ", t->left);
  printf("right %lx\n", t->right);

  if (t->type == NODE_CINT)
    return lval_num((long)t->value);

  char op = (char)t->type;
  lval x = eval(t->value);
  t = t->value->next;

  while (t != NULL) 
  {
    x = eval_op(x, &op, eval(t));
    t = t->next;
  }

  return x;
}

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

    /* Add line to history */
    add_history(input);

    /* Parse */
    YY_BUFFER_STATE buffer = yy_scan_string(input);
    yyparse();
    yy_delete_buffer(buffer);

    /* Free buffer */
    free(input);

    /* Perform calculation */
    lval result = eval(root);
    lval_println(result);
  }

  return 0;
}

