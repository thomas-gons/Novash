/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__


#define _DEFAULT_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include "utils/memory.h"
#include "executor/jobs.h"
#include "executor/builtin.h"
#include "parser/parser.h"


/**
 * @brief Main execution entry point. Traverses the AST and dispatches execution
 * based on the node type (pipeline, command, logical operator).
 * @param ast_node The root node of the Abstract Syntax Tree.
 * @return int The exit status of the executed command or operation.
 */
int exec_node(ast_node_t *ast_node);

#endif // __EXECUTOR_H__