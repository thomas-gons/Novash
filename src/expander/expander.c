#include "expander.h"


static char **expand_argv(word_part_t **argv_parts) {
    char **argv = NULL;
    for (int i = 0; i < arrlen(argv_parts); i++) {
        arrpush(argv, expand_run_pipeline(argv_parts[i]));
    }
    arrpushnc(argv, NULL);
    return argv;
}

static char *expand_redir_target(word_part_t *target_parts) {
    if (!target_parts) return NULL;
    
    return expand_run_pipeline(target_parts);
}

static void expander_expand_cmd(ast_node_t *node) {
    if (node->cmd.argv_parts) {
        arrfree(node->cmd.argv);
        node->cmd.argv = expand_argv(node->cmd.argv_parts);
    }

    if (node->cmd.redir) {
        for (int i = 0; i < arrlen(node->cmd.redir); i++) {
            redirection_t *r = &node->cmd.redir[i];
            if (r->target_parts) {
                free(r->target);
                r->target = expand_redir_target(r->target_parts);
            }
        }
    }
}

void expander_expand_ast(ast_node_t *node) {
    if (!node) return;

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