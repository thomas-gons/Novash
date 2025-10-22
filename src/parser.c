/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "parser.h"


static token_t g_tok = {.type=TOK_EOF, .value=NULL};

/**
 * @brief Simple wrapper to get the next token from the tokenizer
 * @param tz pointer to the tokenizer
 */
static inline void next_token(tokenizer_t *tz) {
    tokenizer_free_token(&g_tok);
    g_tok = tokenizer_next_token(tz);
}

/**
 * @brief Parse the command and its args
 * Duplicates each argument string into argv_buf, resizing it as needed.
 * The array is NULL-terminated for exec-family functions.
 * 
 * @param tz            Tokenizer instance.
 * @return Number of parsed arguments (excluding NULL).
 */
static char **parse_arguments(tokenizer_t *tz) {
    char **argv = NULL;

    while (g_tok.type == TOK_WORD) {
        // the global token value is continually overwritten thus we need to xstrdup
        arr_push(argv, strdup(g_tok.value));
        next_token(tz);
    }
    
    // ensure argv is NULL-terminated for execvp calls later
    // resize if necessary
    arr_push(argv, NULL);
    return argv;
}

/**
 * @brief brief Parse I/O redirections (e.g., <, >, >>, 2>) from the tokenizer.
 * Fills redir_buf with redirection entries, resizing as needed.
 * Each target filename is duplicated for later use.
 * 
 * @param tz              Tokenizer instance.
 * @throw exit the program if the redirection is malformed
 * @return Number of parsed redirections.
 */
static redirection_t *parse_redirection(tokenizer_t *tz) {
    redirection_t *redir = NULL;

    // parse redirections that should appear as : [FD] REDIR_TYPE FILENAME
    while (g_tok.type == TOK_FD || g_tok.type == TOK_REDIR_IN || 
           g_tok.type == TOK_REDIR_OUT || g_tok.type == TOK_REDIR_APPEND)
    {
        redirection_t r = {0};

        if (g_tok.type == TOK_FD) {
            // no need to check for errors here as tokenizer would have handled it
            r.fd = (int) strtol(g_tok.value, NULL, 10);
            next_token(tz);
        }

        // determine redirection type and set default fd if not specified
        switch (g_tok.type) {
            case TOK_REDIR_IN: 
                r.type = REDIR_IN;
                break;
            case TOK_REDIR_OUT: {
                r.type = REDIR_OUT;
                if (r.fd == 0) r.fd = 1;
                break;
            }
            case TOK_REDIR_APPEND: {
                r.type = REDIR_APPEND;
                if (r.fd == 0) r.fd = 1;
                break;
            }
            default:
                fprintf(stderr, "Unexpected token type %d in redirection\n", g_tok.type);
                exit(EXIT_FAILURE);
        }

        next_token(tz);

        if (g_tok.type != TOK_WORD) {
            fprintf(stderr, "Expected filename after redirection\n");
            exit(EXIT_FAILURE);
        }

        // same reason as argv, need to xstrdup
        r.target = xstrdup(g_tok.value);
        arr_push(redir, r);
        next_token(tz);
    }
    return redir;
} 

/**
 * @brief Parse a simple command from the tokenizer
 * @param tz pointer to the tokenizer
 * @return pointer to the parsed AST node representing the command
 */
static ast_node_t *parse_command(tokenizer_t *tz) {
    // safety check
    if (g_tok.type != TOK_WORD) return NULL;

    // will be used to extract the entire raw command string later
    size_t raw_str_start = tz->pos - strlen(g_tok.value);

    char **argv = parse_arguments(tz);    

    redirection_t *redir = parse_redirection(tz);

    size_t raw_str_size = tz->pos - raw_str_start;
    // since the tokenizer stops on a token outside the command,it must be removed
    raw_str_size -= g_tok.raw_length;
    char *raw_str = xmalloc(raw_str_size);
    memcpy(raw_str, tz->input, raw_str_size);
    // strip the trailing space if present
    if (raw_str[raw_str_size - 1] == ' ') raw_str[raw_str_size - 1] = '\0';

    bool is_bg = g_tok.type == TOK_BG;

    ast_node_t *ast_node = xmalloc(sizeof(ast_node_t));
    ast_node->cmd = (cmd_node_t) {
        .argv=argv,
        .redir=redir,
        .raw_str=raw_str,
        .is_bg=is_bg
    };

    ast_node->type = NODE_CMD;
    return ast_node;
}

/**
 * @brief Parse a pipeline of commands connected by '|'.
 * @param tz pointer to the tokenizer
 * @return pointer to the parsed AST node representing the pipeline
 */
static ast_node_t *parse_pipeline(tokenizer_t *tz) {
    ast_node_t *left = parse_command(tz);
    
    // Loop to handle multiple piped commands (e.g., cmd1 | cmd2 | cmd3)
    while (g_tok.type == TOK_PIPE) {
        next_token(tz);
        ast_node_t *right = parse_command(tz);
        ast_node_t *node = xmalloc(sizeof(ast_node_t));
        node->type = NODE_PIPELINE;
        node->pipe.left = left;
        node->pipe.right = right;
        left = node;
    }
    return left;
}

/**
 * @brief Parse a conditional command (&& or ||) using the tokenizer to determine the operator.
 * @param tz pointer to the tokenizer
 * @return pointer to the parsed AST node representing the conditional command
 */
static ast_node_t *parse_conditional(tokenizer_t *tz) {
    ast_node_t *left = parse_pipeline(tz);
    
    // Loop to handle multiple conditionals (e.g., cmd1 && cmd2 || cmd3)
    while (g_tok.type == TOK_AND || g_tok.type == TOK_OR) {
        cond_op_e op = g_tok.type == TOK_AND ? COND_AND: COND_OR;
        next_token(tz);
        ast_node_t *right = parse_pipeline(tz);
        ast_node_t *node = xmalloc(sizeof(ast_node_t));
        node->type = NODE_CONDITIONAL;
        node->cond.left = left;
        node->cond.right = right;
        node->cond.op = op;
        left = node;
    }
    return left;
}

ast_node_t *parser_create_ast(tokenizer_t *tz) {
    next_token(tz);

    if (g_tok.type == TOK_EOF) {
        // g_tok.value is empty no need to free memory
        return NULL;
    }

    ast_node_t *root_node = xmalloc(sizeof(ast_node_t));
    // Initial allocation for dynamic array of sequence nodes
    root_node->type = NODE_SEQUENCE;
    root_node->seq.nodes = NULL;

    // using do-while to ensure at least the first node is added
    do {
        ast_node_t *next_command = parse_conditional(tz);
        if (next_command == NULL) {
            break;
        }

        arr_push(root_node->seq.nodes, next_command);

        if (g_tok.type == TOK_SEMI || g_tok.type == TOK_BG) {
            next_token(tz);
        }
    } while (g_tok.type != TOK_EOF);

    assert(g_tok.type == TOK_EOF);
    tokenizer_free_token(&g_tok);
    
    return root_node;
}

void parser_free_ast(ast_node_t *node) {
    // safety check
    if (!node) return;

    switch (node->type) {
        case NODE_CMD: {
            // free all args, redirections, and raw_str
            cmd_node_t cmd = node->cmd;
            for (size_t i = 0; i < arr_len(cmd.argv); i++) {
                free(cmd.argv[i]);
            }
            arr_free(cmd.argv);
            for (size_t i = 0; i < arr_len(cmd.redir); i++) {
                free(cmd.redir[i].target);
            }
            arr_free(cmd.redir);
            free(cmd.raw_str);
            break;
        }
        case NODE_PIPELINE:
        case NODE_CONDITIONAL:
            parser_free_ast(node->cond.left);
            parser_free_ast(node->cond.right);
            break;
        case NODE_SEQUENCE:
            // free sequence nodes array
            for (size_t i = 0; i < arr_len(node->seq.nodes); i++) {
                parser_free_ast(node->seq.nodes[i]);
            }
            arr_free(node->seq.nodes);
            break; 
        default:
            return;
    }
    free(node);
}

void print_ast(ast_node_t *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_CMD:
            printf("CMD: %s", node->cmd.argv[0]);
            
            // print all arguments
            for (size_t i = 1; i < arr_len(node->cmd.argv); i++)
                printf(" %s", node->cmd.argv[i]);

            // print redirections if any
            if (arr_len(node->cmd.redir) > 0) {
                printf(" [");
                for (size_t i = 0; i < arr_len(node->cmd.redir); i++) {
                    redirection_t r = node->cmd.redir[i];
                    printf("%d", r.fd);
                    switch (r.type) {
                        case REDIR_IN: printf("<"); break;
                        case REDIR_OUT: printf(">"); break;
                        case REDIR_APPEND: printf(">>"); break;
                    }
                    printf("%s", r.target);
                    if (i < arr_len(node->cmd.redir) - 1) printf(", ");
                }
                printf("]");
            }

            // add & if background execution
            if (node->cmd.is_bg) printf(" &");
            printf("\n");
            break;
        
        // for other node types, print their type and recursively
        // print children until cmd nodes are reached
        case NODE_PIPELINE:
            printf("PIPELINE\n");
            print_ast(node->pipe.left, indent + 1);
            print_ast(node->pipe.right, indent + 1);
            break;
        
        case NODE_CONDITIONAL:
            printf("CONDITIONAL (%s)\n", node->cond.op == COND_AND ? "&&" : "||");
            print_ast(node->cond.left, indent + 1);
            print_ast(node->cond.right, indent + 1);
            break;
        
        case NODE_SEQUENCE:
            printf("SEQUENCE\n");
            for (size_t i = 0; i < arr_len(node->seq.nodes); i++) {
                print_ast(node->seq.nodes[i], indent + 1);
            }
            break;
    }
}
