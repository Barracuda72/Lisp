#ifndef __LASSERT_H__
#define __LASSERT_H__
/*
 * Lisp Assertion Macros
 */

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

#endif // __LASSERT_H__
