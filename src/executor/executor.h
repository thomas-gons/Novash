/*
 * Novash — Minimalist shell implementation
 * Executor module: executes AST nodes (commands, pipelines, sequences, conditionals)
 *
 * Copyright (C) 2025 Thomas Gons
 * Licensed under the GNU General Public License v3 or later
 * https://www.gnu.org/licenses/
 */

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "utils/log.h"
#include "utils/xsys.h"
#include "utils/memory.h"
#include "shell/signal.h"
#include "builtin/builtin.h"
#include "executor/jobs.h"
#include "parser/parser.h"

/**
 * @brief Execute an AST node.
 * Traverses the node and dispatches execution according to its type
 * (pipeline, command, logical operator, etc.).
 * @param ast_node Root AST node to execute
 * @return Exit status of the command
 */
int exec_node(ast_node_t *ast_node);

#endif // __EXECUTOR_H__
