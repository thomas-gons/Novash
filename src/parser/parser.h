/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include "lexer/lexer.h"
#include "utils/collections.h"
#include "utils/log.h"
#include "utils/system/memory.h"


typedef enum { REDIR_IN, REDIR_OUT, REDIR_APPEND, REDIR_NONE } redirection_e;

/**
 * Structure representing a redirection in a command.
 * @note target is a dynamically allocated string and should be freed
 * appropriately.
 */
typedef struct {
  int fd; /**<  File descriptor to redirect. (0. stdin, 1. stdout, 2. stderr)*/
  redirection_e type;
  word_part_t *target_parts;
  char *target;
} redirection_t;

/**
 * AST node representing a simple command with arguments and redirections.
 * The raw_str field holds the original command string for reference.
 * The boolean is_bg indicates if the command should run in the background.
 * @note The argv array, redir array and raw_str are dynamically allocated and
 * should be freed appropriately.
 */
typedef struct {
  word_part_t **argv_parts;
  char **argv; /**<  Argument vector (command and its arguments,
                  NULL-terminated). */
  redirection_t *redir;
  char *raw_str; /**<  Original command string for reference. */
  bool is_bg;    /**<  Indicates if the command will run in the background. */
} cmd_node_t;

/**
 * @brief AST node representing a pipeline of commands connected by '|'.
 */
typedef struct {
  struct ast_node_t **nodes;
} pipe_node_t;

typedef enum { COND_AND, COND_OR } cond_op_e;

/**
 * @brief AST node representing a conditional command (&& or ||).
 */
typedef struct {
  struct ast_node_t *left;
  struct ast_node_t *right;
  cond_op_e op;
} cond_node_t;

/**
 * @brief Top level AST node representing a sequence of commands separated by
 * ';' or '&'. For easier traversal and execution, all commands are stored in a
 * dynamic array.
 * @note This array and its contents should be freed appropriately
 */
typedef struct {
  struct ast_node_t **nodes;
} seq_node_t;

/**
 * @brief Enumeration of AST node types to distinguish ast_node_t variants.
 */
typedef enum {
  NODE_CMD,
  NODE_PIPELINE,
  NODE_CONDITIONAL,
  NODE_SEQUENCE
} ast_node_type_e;

/**
 * @brief Abstract Syntax Tree (AST) node structure.
 * Represents different types of nodes: command, pipeline, conditional, and
 * sequence.
 */
typedef struct ast_node_t {
  ast_node_type_e type;
  union {
    cmd_node_t cmd;
    pipe_node_t pipe;
    cond_node_t cond;
    seq_node_t seq;
  };
} ast_node_t;

/**
 * @brief Parses the input tokens into an Abstract Syntax Tree (AST)
 * representing a sequence of commands. Recursively calls other parsing
 * functions according to the grammar.
 * @param lex Pointer to the lexer.
 * @return Pointer to the root AST node representing the sequence.
 */
ast_node_t *parser_create_ast(lexer_t *lex);

/**
 * @brief Frees the memory allocated for the AST.
 * Recursively frees all child nodes and associated resources.
 * @param node Pointer to the root AST node to free.
 * @note this function should be called after the execution of the input command
 */
void parser_free_ast(ast_node_t *node);

/**
 * @brief Prints the AST in a human-readable format for debugging purposes.
 * @param node Pointer to the root AST node.
 * @param indent Current indentation level for pretty-printing.
 * Mostly used for recursive calls but can be set to 0 initially.
 */
char *parser_ast_str(ast_node_t *node, int indent);

#endif // __PARSER_H__