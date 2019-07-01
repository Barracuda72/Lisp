#ifndef __LVAL_H__
#define __LVAL_H__
/*
 * Everything about Lisp Values
 */

#include "common.h"
#include "builtins.h"

#include "tree.h"

/* Possible lval types */
typedef enum 
{ 
  LVAL_STR, // String
  LVAL_FUN, // Function
  LVAL_NUMBER, // Integer
  LVAL_FNUMBER, // Double
  LVAL_ERROR, // Error
  LVAL_SYM, // Symbol (variable)
  LVAL_SEXPR, // S-expression
  LVAL_QEXPR // Q-expression
} lval_type_t;

/* Structure that holds value of operation */
typedef struct _lval
{
  lval_type_t type;
  union {
    long num;
    char* err;
    char* sym;
    char* str;
    double fnum;
    struct {
      lbuiltin builtin;
      const char* name;
      int is_builtin;
      lenv* env;
      lval* formals;
      lval* body;
    };
    struct {
      int count;
      struct _lval** cell;
    };
  };
} lval;

/* Create number */
lval* lval_num(long x);

/* Create floating-point number */
lval* lval_fnum(double x);

/* Create error */
lval* lval_err(const char* fmt, ...);

/* Create symbol */
lval* lval_sym(const char* x);

/* Create S-expression */
lval* lval_sexpr(void);

/* Create Q-expression */
lval* lval_qexpr(void);

/* Create function */
lval* lval_fun_ex(lbuiltin f, const char* name, int builtin);

lval* lval_fun(lbuiltin f);

/* Create lambda */
lval* lval_lambda(lval* formals, lval* body);

/* Create string */
lval* lval_str(const char* s);

/* Clear memory occupied by lval */
void lval_del(lval* v);

/* Create a copy of lval */
lval* lval_copy(lval* v);

lval* lval_read_num(tree* t);

lval* lval_read_fnum(tree* t);

lval* lval_read_str(tree* t);

lval* lval_add(lval* v, lval* x);

lval* lval_read(tree* t);

const char* ltype_name(lval_type_t t);

void lval_expr_print(lval* v, char open, char close);

/* Print string */
void lval_print_str(lval* v);

/* Print lval */
void lval_print(lval* v);

/* Print lval and newline */
void lval_println(lval* v);

lval* lval_pop(lval* v, int i);

lval* lval_take(lval* v, int i);

int lval_eq(lval* x, lval* y);

lval* lval_join(lval* x, lval* y);

/* Comparison */
int lval_less(lval* a, lval* b);

lval* lval_eval_sexpr(lenv* e, lval* v);

lval* lval_call(lenv* e, lval* f, lval* a);

lval* lval_eval(lenv* e, lval* v);

#endif // __LVAL_H__
