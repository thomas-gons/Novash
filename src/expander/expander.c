#include "expander.h"


static void expander_expand_cmd(ast_node_t *node) {
    if (node->cmd.argv_parts) {
        // Expand argv_parts into argv
        arrfree(node->cmd.argv);
        node->cmd.argv = NULL;
        for (int i = 0; i < arrlen(node->cmd.argv_parts); i++) {
            for (int j = 0; j < arrlen(node->cmd.argv_parts[i]); j++) {
                word_part_t part = node->cmd.argv_parts[i][j];
                if (part.type == WORD_LITERAL) {
                    arrpush(node->cmd.argv, xstrdup(part.value));
                } else if (part.type == WORD_VARIABLE) {
                    char *expanded = shell_state_getenv(part.value);
                    if (expanded) {
                        arrpush(node->cmd.argv, expanded);
                    }
                }
            }
        }
    }

    if (node->cmd.redir) {
        for (int i = 0; i < arrlen(node->cmd.redir); i++) {
            redirection_t *redir = &node->cmd.redir[i];
            if (redir->target_parts) {
                arrfree(redir->target);
                redir->target = NULL;
                for (int j = 0; j < arrlen(redir->target_parts); j++) {
                    word_part_t part = redir->target_parts[j];
                    if (part.type == WORD_LITERAL) {
                        redir->target = xstrdup(part.value);
                    } else if (part.type == WORD_VARIABLE) {
                        char *expanded = shell_state_getenv(part.value);
                        if (expanded) {
                            redir->target = xstrdup(expanded);
                        }
                    }
                }
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