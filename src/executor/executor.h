/*
 * Novash — Minimalist shell implementation
 * Executor module: executes AST nodes (commands, pipelines, sequences,
 * conditionals)
 *
 * Copyright (C) 2025 Thomas Gons
 * Licensed under the GNU General Public License v3 or later
 * https://www.gnu.org/licenses/
 */

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#define _DEFAULT_SOURCE

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include "builtin/builtin.h"
#include "executor/jobs.h"
#include "parser/parser.h"
#include "shell/signal.h"
#include "utils/log.h"
#include "utils/system/memory.h"
#include "utils/system/syscall.h"

typedef struct executor_ctx_t {
  pid_t pgid;
  int sfd;
  sigset_t mask, prev_mask;
  int in_fd;
  int out_fd;
  int sync_pipe[2];
} executor_ctx_t;

int handle_foreground_execution(job_t *job, int sfd);

/**
 * @brief Execute an AST node.
 * Traverses the node and dispatches execution according to its type
 * (pipeline, command, logical operator, etc.).
 * @param ast_node Root AST node to execute
 * @return Exit status of the command
 */
int exec_node(ast_node_t *ast_node);

#endif // __EXECUTOR_H__
