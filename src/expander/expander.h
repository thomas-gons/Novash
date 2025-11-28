/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#ifndef NOVASH_EXPANDER_H
#define NOVASH_EXPANDER_H

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pipeline.h"
#include "shell/state.h"

void expander_expand_ast(ast_node_t *node);

#endif // NOVASH_EXPANDER_H