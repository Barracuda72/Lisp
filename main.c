#include <stdio.h>

/* Buffer to hold user input */
#define BUFFER_LEN 2048
static char input[BUFFER_LEN];

int main(int argc, char* argv[])
{
  fputs ("Lisp version 0.0.1\n", stdout);
  fputs ("Press Ctrl+C to exit prompt\n\n", stdout);

  /* Never-ending prompt */
  while (1)
  {
    /* Output prompt */
    fputs ("lisp> ", stdout);

    /* Read user input */
    fgets (input, BUFFER_LEN, stdin);

    /* Forcefully terminate it */
    input[BUFFER_LEN] = 0;

    /* Output it back */
    /* TODO: better use something that not's printf! */
    printf("No you're %s", input);
  }

  return 0;
}
