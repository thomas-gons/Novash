/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#include "expander.h"

static void expander_expand_cmd(ast_node_t *node) {
  if (node->cmd.argv_parts) {
    arrfree(node->cmd.argv);
    node->cmd.argv = expand_argv_parts(node->cmd.argv_parts);
  }

  if (node->cmd.redir) {
    for (int i = 0; i < arrlen(node->cmd.redir); i++) {
      redirection_t *r = &node->cmd.redir[i];
      if (r->target_parts) {
        free(r->target);
        r->target = expand_redirection_target(r->target_parts);
      }
    }
  }
}

void expander_expand_ast(ast_node_t *node) {
  if (!node)
    return;

  switch (node->type) {
  case NODE_CMD:
    expander_expand_cmd(node);
    break;
  case NODE_PIPELINE:
    for (int i = 0; i < arrlen(node->pipe.nodes); i++)
      expander_expand_ast(node->pipe.nodes[i]);
    break;
  case NODE_CONDITIONAL:
    expander_expand_ast(node->cond.left);
    expander_expand_ast(node->cond.right);
    break;
  case NODE_SEQUENCE:
    for (int i = 0; i < arrlen(node->seq.nodes); i++)
      expander_expand_ast(node->seq.nodes[i]);
    break;
  }
}