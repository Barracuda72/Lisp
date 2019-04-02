%{
#include <stdio.h>
#include <string.h>

#include "yystype.h"

#include "y.tab.h"

void yyerror(const char *s);

int yycharpos = 1;

#define YY_USER_ACTION yycharpos += yyleng;

#define BUFFER_SIZE 1024
static char buffer[BUFFER_SIZE];
static const char* strdup(const char* s)
{
  strncpy(buffer, s, BUFFER_SIZE);
  buffer[BUFFER_SIZE-1] = 0;
  return buffer;
}
#undef BUFFER_SIZE

%}

%option case-insensitive
%option yylineno

white_space       [ \t]*
digit             [0-9]
alpha             [A-Za-z_]
alpha_num         ({alpha}|{digit})
hex_digit         [0-9A-Fa-f]
identifier        {alpha}{alpha_num}*
hex_integer       {hex_digit}{hex_digit}*H
unsigned_integer  {digit}+
integer           [+-]?{unsigned_integer}
exponent          e[+-]?{digit}+
i                 {unsigned_integer}
real              ({i}\.{i}?|{i}?\.{i}){exponent}?
string            \'([^'\n]|\'\'|\\\')*\'
string2           \"([^"\n]|\"\"|\\\")*\"
comment           ;[^\n]*

%%

{real}               yylval = strdup(yytext); return (TOK_REAL);
{integer}            yylval = strdup(yytext); return (TOK_INTEGER);

[()]                 return (yytext[0]);
[%+*/-]              yylval = strdup(yytext); return (TOK_SYMBOL);

{white_space}        /* Skip whitespaces */
{comment}            /* Skip comments */
\n                   yycharpos = 1;
.                    yyerror("Illegal input!");

%%
