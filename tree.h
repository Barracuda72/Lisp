#ifndef __TREE_H__
#define __TREE_H__

#define __need_NULL
#include <stddef.h>

/*
 * Tree structure and operations on it
 */


/*
 * Tree structure that can be used as list
 */
typedef struct _tree {
  int type;
  int id;

  union {
    struct _tree *left;
    struct _tree *value;
  };

  union {
    struct _tree *right;
    struct _tree *next;
  };

  void *param;
} tree;

/*
 * For loop directions
 */
enum {
  FOR_DIR_FWD = 1,
  FOR_DIR_BWD = 2,
};

/*
 * Node types
 */
enum {
  /*
   * Node types in expression
   * for multi-character operations
   * Single-character ('+', '/', etc) present with
   * corresponding characters
   */
  NODE_OP_START     =  256,
  NODE_OP_EQ        =  300, 
  NODE_OP_NE        =  301,
  NODE_OP_LE        =  302,
  NODE_OP_GE        =  303,
  NODE_OP_LOR       =  304, /* Logical OR */
  NODE_OP_LAND      =  305, /* Logical AND */
  NODE_OP_NOT       =  306, /* Logical NOT */
  MODE_OP_LIMIT     =  999, /* Limit for operational nodes */

  /*
   * Generic node types
   */
  NODE_FUNC_NAME    = 1000,
  NODE_CLASS_NAME   = 1001,
  NODE_STMT_LIST    = 1002,
  NODE_IF_STMT      = 1003,
  NODE_FOR_STMT     = 1004,
  NODE_WHILE_STMT   = 1005,
  NODE_UNTIL_STMT   = 1006,
  NODE_RETURN_STMT  = 1007,
  NODE_BREAK_STMT   = 1008,
  NODE_VAR_DECL     = 1009,
  NODE_CONST_DECL   = 1010,
  NODE_ASSIGN       = 1011,
  NODE_FUNC_DEF     = 1012,
  NODE_CLASS_DEF    = 1013,
  NODE_EXTERN_DECL  = 1014,
  NODE_CLASS_DECL   = 1015,
  NODE_ARRAY_USE    = 1016,
  NODE_FIELD_USE    = 1017,
  NODE_FUNC_CALL    = 1018,
  NODE_METH_INVOKE  = 1019,
  NODE_IDENTIFIER   = 1020,
  NODE_ID_TYPE      = 1021,

  /*
   * Nodes for constants
   */
  NODE_CSTRING      = 2000,
  NODE_CINT         = 2001,
  NODE_CFLOAT       = 2002,
};

/*
 * Function type
 */
enum
{
  FTYPE_GENERIC     = 3000,
  FTYPE_CONSTRUCTOR = 3001,
};

/*
 * Create node with parameters
 */
tree *tree_create_ex(tree *left, tree *right, int type, int id, void *param);

/*
 * Simplify node creation
 */
tree *tree_create(tree *left, tree *right, int type);

/*
 * Link two lists together
 */
tree *tree_link_lists(tree *head, tree *tail);

// HACK: definition for operation dispatcher
#define OP_DISPATCHER "op_dispatcher"

#endif // __TREE_H__
