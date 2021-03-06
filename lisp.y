%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree.h"

#include "yystype.h"

extern char *yytext;
extern int yylineno;
extern int yycharpos;
extern int yyleng;
extern FILE *yyin;

static char** pool = NULL;
static int pool_size = 0;

char* mystrdup(const char* s)
{
  pool = (char**)realloc(pool, sizeof(char*)*(pool_size+1));
  char* d = (char*)malloc(strlen(s)+1);
  strcpy(d, s);
  pool[pool_size] = d;
  pool_size++;
  return d;
}

void parser_free_pool(void)
{
  for (int i = 0; i < pool_size; i++)
    free(pool[i]);

  free(pool);

  pool = NULL;
  pool_size = 0;
}

void yyerror(const char *s)
{
  int c = yycharpos - yyleng;

  char *t = malloc(2048);
  sprintf(t, "^ Error: \"%s\" at %d:%d, got '%s'",
    s, yylineno, c, yytext);
#if 0
  int i,k;
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
  TOK_INTEGER TOK_REAL TOK_SYMBOL TOK_STRING

%%

/* 
 * Program is an one or more expressions
 */
program:
  expression_list { 
    root = tree_create((tree*)$1, NULL, NODE_VAR_DECL);
    $$ = (YYSTYPE)root;
  }
  ;

expression_list:
  /* empty */ { $$ = (YYSTYPE)NULL; } |
  expression expression_list { $$ = (YYSTYPE)tree_link_lists(tree_create((tree*)$1, NULL, NODE_STMT_LIST), (tree*)$2); }
  ;

expression:
  number { $$ = $1; } |
  symbol { $$ = $1; } |
  string { $$ = $1; } |
  sexpression { $$ = $1; } |
  qexpression { $$ = $1; }
  ;

qexpression:
  '{' expression_list '}' { $$ = (YYSTYPE)tree_create((tree*)$2, NULL, NODE_CONST_DECL); }
  ;

sexpression:
  '(' expression_list ')' { $$ = (YYSTYPE)tree_create((tree*)$2, NULL, NODE_VAR_DECL); }
  ;

symbol:
  TOK_SYMBOL { $$ = (YYSTYPE)tree_create((tree*)mystrdup((char*)$1), NULL, NODE_IDENTIFIER); }
  ;

string:
  TOK_STRING { $$ = (YYSTYPE)tree_create((tree*)mystrdup((char*)$1), NULL, NODE_CSTRING); }
  ;

number:
  TOK_INTEGER { $$ = (YYSTYPE)tree_create((tree*)mystrdup((char*)$1), NULL, NODE_CINT); } |
  TOK_REAL
  {
    $$ = (YYSTYPE)tree_create((tree*)mystrdup((char*)$1), NULL, NODE_CFLOAT);
  }
  ;

