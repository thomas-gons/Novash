/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __LEXER_H__
#define __LEXER_H__

#include "utils/system/memory.h"
#include "utils/collections.h"
#include "utils/log.h"
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#define MAX_TOKEN_SIZE 512

typedef enum {
  TOK_WORD,
  TOK_SEMI,
  TOK_PIPE,
  TOK_OR,
  TOK_AND,
  TOK_BG,
  TOK_FD,
  TOK_REDIR_IN,
  TOK_REDIR_OUT,
  TOK_REDIR_APPEND,
  TOK_EOF,
} token_type_e;

typedef enum {
  WORD_LITERAL,
  WORD_VARIABLE,
  WORD_TILDE,
  WORD_GLOB
} word_part_type_e;

typedef enum {
  QUOTE_NONE,
  QUOTE_SINGLE,
  QUOTE_DOUBLE
} quote_context_e;


typedef struct {
  word_part_type_e type;
  quote_context_e quote;
  char *value;
} word_part_t;

/**
 * Token structure representing a lexical token.
 * The value field is a dynamically allocated string for tokens that carry
 * values: commands, arguments, and file descriptors for redirections. It should
 * be freed appropriately.
 */
typedef struct {
  token_type_e type;
  char *raw_value;
  word_part_t *parts;
} token_t;

typedef struct {
  char *input;
  size_t pos;
  size_t length;
} lexer_t;

/**
 * create a new lexer
 * @return pointer to the new lexer
 */
lexer_t *lexer_new();

/**
 * init all the fields of the lexer
 * @param lex the lexer
 */
void lexer_init(lexer_t *lex, char *input);

/**
 * free the lexer and its resources
 * @param lex pointer to the lexer to free
 */
void lexer_free(lexer_t *lex);

/**
 * free the token's value if it is non null
 * @param tok pointer to the token to free
 */
void lexer_free_token(token_t *tok);

/**
 * get the next token from the input
 * @param lex pointer to the lexer
 * @return the next token
 */
token_t lexer_next_token(lexer_t *lex);

/**
 * print the token type and value for debugging
 * @param tok the token to print
 * @param buf buffer to write the string representation limited to buf_sz
 * @param buf_sz size of the buffer
 * @return number of characters that would have been written
 */
size_t lexer_token_str(token_t tok, char *buf, size_t buf_sz);


const char *lexer_part_type_str(word_part_type_e type);

#endif // __LEXER_H__