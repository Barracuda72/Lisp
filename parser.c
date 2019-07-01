/*
 * Parser stuff
 */

#include <stdlib.h>

#include "parser.h"

extern tree* root;

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

tree* parse_and_free(char* input)
{
  /* Parse */
  YY_BUFFER_STATE buffer = yy_scan_string(input);
  yyparse();
  yy_delete_buffer(buffer);

  /* Free buffer */
  free(input);

  return root;
}
