#ifndef __EXPANDER_H__
#define __EXPANDER_H__

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "shell/state.h"


void expander_expand_ast(ast_node_t *node);

#endif // __EXPANDER_H__