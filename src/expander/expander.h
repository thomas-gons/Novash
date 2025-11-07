#ifndef NOVASH_EXPANDER_H
#define NOVASH_EXPANDER_H

#include "pipeline.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "shell/state.h"

void expander_expand_ast(ast_node_t *node);

#endif // NOVASH_EXPANDER_H