#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

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

  /* Never-ending prompt */
  while (1)
  {
    /* Output prompt and read user input */
    char* input = readline("lisp> ");

    /* Add line to history */
    add_history(input);

    /* Output it back */
    /* TODO: better use something that not's printf! */
    printf("No you're %s\n", input);

    /* Free buffer */
    free(input);
  }

  return 0;
}
