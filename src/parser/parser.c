/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "parser.h"

static token_t g_tok = {.type = TOK_EOF, .raw_value = NULL, .parts = NULL};

/**
 * @brief Simple wrapper to get the next token from the lexer
 * @param lex pointer to the lexer
 */
static inline void next_token(lexer_t *lex) {
  lexer_free_token(&g_tok);
  g_tok = lexer_next_token(lex);
}

static word_part_t *duplicate_word_parts(word_part_t *parts) {
  if (!parts)
    return NULL;

  word_part_t *dup_parts = NULL;
  for (int i = 0; i < arrlen(parts); i++) {
    word_part_t p = parts[i];
    word_part_t p_dup = {
        .type = p.type,
        .quote = p.quote,
        .value = xstrdup(p.value),
    };
    arrpush(dup_parts, p_dup);
  }
  return dup_parts;
}

/**
 * @brief Parse the command and its args
 * Duplicates each argument string into argv_buf, resizing it as needed.
 * The array is NULL-terminated for exec-family functions.
 *
 * @param lex            lexer instance.
 * @return Number of parsed arguments (excluding NULL).
 */
static word_part_t **parse_arguments(lexer_t *lex) {
  word_part_t **argv_parts = NULL;

  while (g_tok.type == TOK_WORD) {
    arrpush(argv_parts, duplicate_word_parts(g_tok.parts));
    next_token(lex);
  }

  return argv_parts;
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
         g_tok.type == TOK_REDIR_OUT || g_tok.type == TOK_REDIR_APPEND) {
    redirection_t r = {0};

    if (g_tok.type == TOK_FD) {
      // no need to check for errors here as lexer would have handled it
      r.fd = (int)strtol(g_tok.raw_value, NULL, 10);
      next_token(lex);
    }

    // determine redirection type and set default fd if not specified
    switch (g_tok.type) {
    case TOK_REDIR_IN:
      r.type = REDIR_IN;
      break;
    case TOK_REDIR_OUT: {
      r.type = REDIR_OUT;
      if (r.fd == 0)
        r.fd = 1;
      break;
    }
    case TOK_REDIR_APPEND: {
      r.type = REDIR_APPEND;
      if (r.fd == 0)
        r.fd = 1;
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
    r.target_parts = duplicate_word_parts(g_tok.parts);
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
  if (g_tok.type != TOK_WORD)
    return NULL;

  size_t start = lex->pos;
  word_part_t **argv_parts = parse_arguments(lex);
  redirection_t *redir = parse_redirection(lex);

  char *raw_str = xmalloc(lex->pos - start + 1);
  strncpy(raw_str, &lex->input[start], lex->pos - start);
  raw_str[lex->pos - start] = '\0';

  bool is_bg = g_tok.type == TOK_BG;

  ast_node_t *ast_node = xmalloc(sizeof(ast_node_t));
  ast_node->type = NODE_CMD;
  ast_node->cmd = (cmd_node_t){
      .argv_parts = argv_parts,
      .argv = NULL,
      .redir = redir,
      .raw_str = raw_str,
      .is_bg = is_bg
  };

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
 * @brief Parse a conditional command (&& or ||) using the lexer to determine
 * the operator.
 * @param lex pointer to the lexer
 * @return pointer to the parsed AST node representing the conditional command
 */
static ast_node_t *parse_conditional(lexer_t *lex) {
  ast_node_t *left = parse_pipeline(lex);

  // Loop to handle multiple conditionals (e.g., cmd1 && cmd2 || cmd3)
  while (g_tok.type == TOK_AND || g_tok.type == TOK_OR) {
    cond_op_e op = g_tok.type == TOK_AND ? COND_AND : COND_OR;
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

    // consume any number of consecutive separators (; or &), e.g. "&;" or ";;"
    // or "&;&"
    while (g_tok.type == TOK_SEMI || g_tok.type == TOK_BG) {
      next_token(lex);
    }
  } while (g_tok.type != TOK_EOF);

  lexer_free_token(&g_tok);

  return root_node;
}

void parser_free_ast(ast_node_t *node) {
  // safety check
  if (!node)
    return;

  switch (node->type) {
  case NODE_CMD: {
    // free all args, redirections, and raw_str
    cmd_node_t cmd = node->cmd;
    for (int i = 0; i < arrlen(cmd.argv_parts); i++) {
      for (int j = 0; j < arrlen(cmd.argv_parts[i]); j++) {
        free(cmd.argv_parts[i][j].value);
      }
      arrfree(cmd.argv_parts[i]);
    }
    arrfree(cmd.argv_parts);
    
    for (int i = 0; i < arrlen(cmd.argv); i++) {
      free(cmd.argv[i]);
    }
    arrfree(cmd.argv);

    for (int i = 0; i < arrlen(cmd.redir); i++) {
      for (int j = 0; j < arrlen(cmd.redir[i].target_parts); j++) {
        free(cmd.redir[i].target_parts[j].value);
      }
      arrfree(cmd.redir[i].target_parts);

      if (cmd.redir[i].target) {
        free(cmd.redir[i].target);
      }
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

static char *raw_part_to_str(word_part_t *part) {
  char *buf = xmalloc(1024);
  const char *type_str = (part->type == WORD_LITERAL) ? "LIT" : "VAR";
  const char *val_str = (part->value) ? part->value : "NULL";
  int len = snprintf(buf, 1024, "[%s] %s ", type_str, val_str);
  buf[len] = '\0';
  return buf;
}

static void rec_ast(ast_node_t *node, int indent, char ***lines) {
  if (!node)
    return;

  char buf[2048];

  switch (node->type) {
  case NODE_CMD: {
    snprintf(buf, sizeof(buf), "%*sCMD:", indent, "");
    arrpush(*lines, xstrdup(buf));

    // argv / argv_parts
    if (node->cmd.argv) {
      snprintf(buf, sizeof(buf), "%*sargv:", indent + 2, "");
      for (int i = 0; i < arrlen(node->cmd.argv); i++) {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), " %s", node->cmd.argv[i]);
        strcat(buf, tmp);
      }
      arrpush(*lines, xstrdup(buf));
    } else if (node->cmd.argv_parts) {
      snprintf(buf, sizeof(buf), "%*sargv_parts:", indent + 2, "");
      arrpush(*lines, xstrdup(buf));
      for (int i = 0; i < arrlen(node->cmd.argv_parts); i++) {
        char *s = raw_part_to_str(node->cmd.argv_parts[i]);
        snprintf(buf, sizeof(buf), "%*s%s", indent + 4, "", s);
        arrpush(*lines, xstrdup(buf));
        free(s);
      }
      snprintf(buf, sizeof(buf), "%*s", indent + 2, "");
      arrpush(*lines, xstrdup(buf));
    }

    // redirections
    if (arrlen(node->cmd.redir) > 0) {
      for (int i = 0; i < arrlen(node->cmd.redir); i++) {
        redirection_t r = node->cmd.redir[i];
        const char *redir_op =
            (r.type == REDIR_IN)     ? "<"
            : (r.type == REDIR_OUT)  ? ">"
            : (r.type == REDIR_APPEND) ? ">>"
                                       : "?";
        if (r.target) {
          snprintf(buf, sizeof(buf), "%*s[%d%s %s]", indent + 2, "", r.fd,
                   redir_op, r.target);
          arrpush(*lines, xstrdup(buf));
        } else if (r.target_parts) {
          snprintf(buf, sizeof(buf), "%*s[%d%s:", indent + 2, "", r.fd,
                   redir_op);
          arrpush(*lines, xstrdup(buf));
          snprintf(buf, sizeof(buf), "%*s[target_parts:", indent + 4, "");
          arrpush(*lines, xstrdup(buf));
          for (int j = 0; j < arrlen(r.target_parts); j++) {
            char *s = raw_part_to_str(&r.target_parts[j]);
            snprintf(buf, sizeof(buf), "%*s%s", indent + 6, "", s);
            arrpush(*lines, xstrdup(buf));
            free(s);
          }
          snprintf(buf, sizeof(buf), "%*s]", indent + 4, "");
          arrpush(*lines, xstrdup(buf));
          snprintf(buf, sizeof(buf), "%*s]", indent + 2, "");
          arrpush(*lines, xstrdup(buf));
        }
      }
    }

    if (node->cmd.is_bg) {
      snprintf(buf, sizeof(buf), "%*s&", indent + 2, "");
      arrpush(*lines, xstrdup(buf));
    }

  } break;

  case NODE_PIPELINE:
    snprintf(buf, sizeof(buf), "%*sPIPELINE", indent, "");
    arrpush(*lines, xstrdup(buf));
    for (int i = 0; i < arrlen(node->pipe.nodes); i++)
      rec_ast(node->pipe.nodes[i], indent + 2, lines);
    break;

  case NODE_CONDITIONAL:
    snprintf(buf, sizeof(buf), "%*sCONDITIONAL (%s)", indent, "",
             node->cond.op == COND_AND ? "&&" : "||");
    arrpush(*lines, xstrdup(buf));
    rec_ast(node->cond.left, indent + 2, lines);
    rec_ast(node->cond.right, indent + 2, lines);
    break;

  case NODE_SEQUENCE:
    snprintf(buf, sizeof(buf), "%*sSEQUENCE", indent, "");
    arrpush(*lines, xstrdup(buf));
    for (int i = 0; i < arrlen(node->seq.nodes); i++)
      rec_ast(node->seq.nodes[i], indent + 2, lines);
    break;
  }
}


char *parser_ast_str(ast_node_t *node, int indent) {
  if (!node)
    return NULL;

  char **lines = NULL;
  rec_ast(node, indent, &lines);

  size_t total_len = 0;
  for (int i = 0; i < arrlen(lines); i++)
    total_len += strlen(lines[i]) + 1;

  char *out = malloc(total_len + 1);
  if (!out)
    exit(EXIT_FAILURE);
  out[0] = '\0';

  for (int i = 0; i < arrlen(lines); i++) {
    strcat(out, lines[i]);
    strcat(out, "\n");
    free(lines[i]);
  }
  arrfree(lines);

  return out;
}
