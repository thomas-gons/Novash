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
 * @brief Simple wrapper to get the next token from the lexer
 * @param lex pointer to the lexer
 */
static inline void next_token(lexer_t *lex) {
    lexer_free_token(&g_tok);
    g_tok = lexer_next_token(lex);
}

/**
 * @brief Parse the command and its args
 * Duplicates each argument string into argv_buf, resizing it as needed.
 * The array is NULL-terminated for exec-family functions.
 * 
 * @param lex            lexer instance.
 * @return Number of parsed arguments (excluding NULL).
 */
static char **parse_arguments(lexer_t *lex) {
    char **argv = NULL;

    while (g_tok.type == TOK_WORD) {
        // the global token value is continually overwritten thus we need to xxstrdup
        arrpush(argv, xstrdup(g_tok.value));
        next_token(lex);
    }
    
    // ensure argv is NULL-terminated for execvp calls later
    // resize if necessary
    return argv;
}

/**
 * @brief brief Parse I/O redirections (e.g., <, >, >>, 2>) from the lexer.
 * Fills redir_buf with redirection entries, resizing as needed.
 * Each target filename is duplicated for later use.
 * 
 * @param lex              lexer instance.
 * @throw exit the program if the redirection is malformed
 * @return Number of parsed redirections.
 */
static redirection_t *parse_redirection(lexer_t *lex) {
    redirection_t *redir = NULL;

    // parse redirections that should appear as : [FD] REDIR_TYPE FILENAME
    while (g_tok.type == TOK_FD || g_tok.type == TOK_REDIR_IN || 
           g_tok.type == TOK_REDIR_OUT || g_tok.type == TOK_REDIR_APPEND)
    {
        redirection_t r = {0};

        if (g_tok.type == TOK_FD) {
            // no need to check for errors here as lexer would have handled it
            r.fd = (int) strtol(g_tok.value, NULL, 10);
            next_token(lex);
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

        next_token(lex);

        if (g_tok.type != TOK_WORD) {
            fprintf(stderr, "Expected filename after redirection\n");
            exit(EXIT_FAILURE);
        }

        // same reason as argv, need to xxstrdup
        r.target = xstrdup(g_tok.value);
        arrpush(redir, r);
        next_token(lex);
    }
    return redir;
} 

/**
 * @brief Parse a simple command from the lexer
 * @param lex pointer to the lexer
 * @return pointer to the parsed AST node representing the command
 */
static ast_node_t *parse_command(lexer_t *lex) {
    // safety check
    if (g_tok.type != TOK_WORD) return NULL;

    // will be used to extract the entire raw command string later
    size_t raw_str_start = lex->pos - strlen(g_tok.value);

    char **argv = parse_arguments(lex);    

    redirection_t *redir = parse_redirection(lex);

    size_t raw_str_size = lex->pos - raw_str_start;
    // since the lexer stops on a token outside the command,it must be removed
    raw_str_size -= g_tok.raw_length;
    char *raw_str = xmalloc(raw_str_size + 1);
    memcpy(raw_str, lex->input + raw_str_start, raw_str_size);
    // strip the trailing space if present
    if (raw_str[raw_str_size - 1] == ' ') {
        raw_str[raw_str_size - 1] = '\0';
    } else {
        raw_str[raw_str_size] = '\0';
    }

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
 * @param lex pointer to the lexer
 * @return pointer to the parsed AST node representing the pipeline
 */
static ast_node_t *parse_pipeline(lexer_t *lex) {
    
    ast_node_t *first_command = parse_command(lex);
    ast_node_t *node = xmalloc(sizeof(ast_node_t));
    node->type = NODE_PIPELINE;
    node->pipe.nodes = NULL;
    arrpush(node->pipe.nodes, first_command);

    // Loop to handle multiple piped commands (e.g., cmd1 | cmd2 | cmd3)
    while (g_tok.type == TOK_PIPE) {
        next_token(lex);
        ast_node_t *next_command = parse_command(lex);
        if (!next_command) {
            fprintf(stderr, "Syntax error: expected command after '|'\n");
            parser_free_ast(node);
            exit(EXIT_FAILURE);
        }
        arrpush(node->pipe.nodes, next_command);
    }

    if (arrlen(node->pipe.nodes) == 1) {
        // single command, no pipeline needed
        ast_node_t *single_cmd = node->pipe.nodes[0];
        arrfree(node->pipe.nodes);
        free(node);
        return single_cmd;
    }

    return node;
}

/**
 * @brief Parse a conditional command (&& or ||) using the lexer to determine the operator.
 * @param lex pointer to the lexer
 * @return pointer to the parsed AST node representing the conditional command
 */
static ast_node_t *parse_conditional(lexer_t *lex) {
    ast_node_t *left = parse_pipeline(lex);
    
    // Loop to handle multiple conditionals (e.g., cmd1 && cmd2 || cmd3)
    while (g_tok.type == TOK_AND || g_tok.type == TOK_OR) {
        cond_op_e op = g_tok.type == TOK_AND ? COND_AND: COND_OR;
        next_token(lex);
        ast_node_t *right = parse_pipeline(lex);
        ast_node_t *node = xmalloc(sizeof(ast_node_t));
        node->type = NODE_CONDITIONAL;
        node->cond.left = left;
        node->cond.right = right;
        node->cond.op = op;
        left = node;
    }
    return left;
}

ast_node_t *parser_create_ast(lexer_t *lex) {
    next_token(lex);

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
        ast_node_t *next_command = parse_conditional(lex);
        if (next_command == NULL) {
            break;
        }

        arrpush(root_node->seq.nodes, next_command);

        // consume any number of consecutive separators (; or &), e.g. "&;" or ";;" or "&;&"
        while (g_tok.type == TOK_SEMI || g_tok.type == TOK_BG) {
            next_token(lex);
        }
    } while (g_tok.type != TOK_EOF);

    lexer_free_token(&g_tok);
    
    return root_node;
}

void parser_free_ast(ast_node_t *node) {
    // safety check
    if (!node) return;

    switch (node->type) {
        case NODE_CMD: {
            // free all args, redirections, and raw_str
            cmd_node_t cmd = node->cmd;
            for (int i = 0; i < arrlen(cmd.argv); i++) {
                free(cmd.argv[i]);
            }
            arrfree(cmd.argv);
            for (int i = 0; i < arrlen(cmd.redir); i++) {
                free(cmd.redir[i].target);
            }
            arrfree(cmd.redir);
            free(cmd.raw_str);
            break;
        }
        case NODE_PIPELINE:
            // free pipeline nodes array
            for (int i = 0; i < arrlen(node->pipe.nodes); i++) {
                parser_free_ast(node->pipe.nodes[i]);
            }
            arrfree(node->pipe.nodes);
            break;
        case NODE_CONDITIONAL:
            parser_free_ast(node->cond.left);
            parser_free_ast(node->cond.right);
            break;
        case NODE_SEQUENCE:
            // free sequence nodes array
            for (int i = 0; i < arrlen(node->seq.nodes); i++) {
                parser_free_ast(node->seq.nodes[i]);
            }
            arrfree(node->seq.nodes);
            break; 
        default:
            return;
    }
    free(node);
}

static void rec_ast(ast_node_t *node, int indent, char ***lines) {
    if (!node) return;

    char buf[1024];

    switch (node->type) {
        case NODE_CMD:
            {
                // indentation
                int pos = snprintf(buf, sizeof(buf), "%*sCMD: %s", indent, "", node->cmd.argv[0]);

                // arguments
                for (int i = 1; i < arrlen(node->cmd.argv); i++)
                    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, " %s", node->cmd.argv[i]);

                // redirections
                if (arrlen(node->cmd.redir) > 0) {
                    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, " [");
                    for (int i = 0; i < arrlen(node->cmd.redir); i++) {
                        redirection_t r = node->cmd.redir[i];
                        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "%d", r.fd);
                        switch (r.type) {
                            case REDIR_IN: pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "<"); break;
                            case REDIR_OUT: pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, ">"); break;
                            case REDIR_APPEND: pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, ">>"); break;
                            default: break;
                        }
                        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "%s", r.target);
                        if (i < arrlen(node->cmd.redir) - 1)
                            pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, ", ");
                    }
                    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]");
                }

                // background
                if (node->cmd.is_bg)
                    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, " &");

                arrpush(*lines, xstrdup(buf));
            }
            break;

        case NODE_PIPELINE:
            snprintf(buf, sizeof(buf), "%*sPIPELINE", indent, "");
            arrpush(*lines, xstrdup(buf));
            for (int i = 0; i < arrlen(node->pipe.nodes); i++)
                rec_ast(node->pipe.nodes[i], indent + 1, lines);
            break;

        case NODE_CONDITIONAL:
            snprintf(buf, sizeof(buf), "%*sCONDITIONAL (%s)", indent, "",
                     node->cond.op == COND_AND ? "&&" : "||");
            arrpush(*lines, xstrdup(buf));
            rec_ast(node->cond.left, indent + 1, lines);
            rec_ast(node->cond.right, indent + 1, lines);
            break;

        case NODE_SEQUENCE:
            snprintf(buf, sizeof(buf), "%*sSEQUENCE", indent, "");
            arrpush(*lines, xstrdup(buf));
            for (int i = 0; i < arrlen(node->seq.nodes); i++)
                rec_ast(node->seq.nodes[i], indent + 1, lines);
            break;
    }
}

char *parser_ast_str(ast_node_t *node, int indent) {
    if (!node) return NULL;

    char **lines = NULL;
    rec_ast(node, indent, &lines);

    size_t total_len = 0;
    for (int i = 0; i < arrlen(lines); i++)
        total_len += strlen(lines[i]) + 1;

    char *out = malloc(total_len + 1);
    if (!out) exit(EXIT_FAILURE);
    out[0] = '\0';

    for (int i = 0; i < arrlen(lines); i++) {
        strcat(out, lines[i]);
        strcat(out, "\n");
        free(lines[i]);
    }
    arrfree(lines);

    return out;
}
