#ifndef __PARSER_H__
#define __PARSER_H__
/*
 * Parser stuff
 */

#include "tree.h"

#ifdef DEBUG
extern int yydebug;
#endif

extern void parser_free_pool(void);

tree* parse_and_free(char* input);

#endif // __PARSER_H__
