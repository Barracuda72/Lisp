#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <errno.h>

#include "tree.h"
#include "lval.h"
#include "lenv.h"
#include "builtins.h"
#include "parser.h"

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

int main(int argc, char* argv[])
{
  fputs (copyright, stdout);
  fputs ("Press Ctrl+C to exit prompt\n\n", stdout);

#ifdef DEBUG
  yydebug = 1;
#endif

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  /* Supplied with list of files */
  if (argc >= 2) {
    /* loop over each supplied filename (starting from 1) */
    for (int i = 1; i < argc; i++) 
    {
      /* Argument list with a single argument, the filename */
      lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
  
      /* Pass to builtin load and get the result */
      lval* x = builtin_load(e, args);
  
      /* If the result is an error be sure to print it */
      if (x->type == LVAL_ERROR) 
        lval_println(x);

      lval_del(x);
    }
  } else {
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
  
      /* Parse string */
      tree* root = parse_and_free(input);
  
      /* Convert input into S-expr */
      lval* x = lval_read(root);
      //lval_println(x);
      x = lval_eval(e, x);
  
      /* Free string pool used by parser */
      parser_free_pool();
  
      /* Perform calculation */
      lval_println(x);
      lval_del(x);
    }
  }

  lenv_del(e);

  return 0;
}

