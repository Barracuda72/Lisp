%{
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tree.h"

#include "yystype.h"

extern char *yytext;
extern int yylineno;
extern int yycharpos;
extern int yyleng;
extern FILE *yyin;

void yyerror(const char *s)
{
  int c = yycharpos - yyleng;
  int i,k;

  char *t = malloc(2048);
  sprintf(t, "^ Error: \"%s\" at %d:%d, got '%s'",
    s, yylineno, c, yytext);
#if 0
  while ((k = getc(yyin)) != EOF)
  {
    putchar(k);
    if ((k == '\n') && (t != 0))
    {
      for (i = 1; i < c; i++)
        putchar('*');
      puts(t);

      free(t);
      t = 0;
    }
  }
#else
  puts(t);
  free(t);
#endif
}

int yywrap()
{
  return 1;
}

tree *root = NULL;

%}

%token
  TOK_INTEGER

%%

/* 
 * Program is an operator and one or more expressions
 */
program:
  operator expression_list { root = tree_create($2, NULL, $1); }
  ;

expression_list:
  /* empty */ { $$ = NULL; } |
  expression expression_list { $$ = tree_link_lists($1, $2); }
  ;

expression:
  number { $$ = $1; } |
  '(' operator expression_list ')' { $$ = tree_create($3, NULL, $2); }
  ;

operator:
  '+' { $$ = '+'; } |
  '-' { $$ = '-'; } |
  '*' { $$ = '*'; } |
  '/' { $$ = '/'; }
  ;

number:
  TOK_INTEGER { $$ = tree_create(strtol($1, NULL, 10), NULL, NODE_CINT); }
  ;

