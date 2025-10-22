/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <stdbool.h>
#include <stdbool.h>
#include <string.h>
#include "utils/memory.h"


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

/**
 * Token structure representing a lexical token.
 * The value field is a dynamically allocated string for tokens that carry values:
 * commands, arguments, and file descriptors for redirections.
 * It should be freed appropriately.
 */
typedef struct {
    token_type_e type;
    char *value;
    size_t raw_length;
} token_t;

typedef struct {
    char *input;
    size_t pos;
    size_t length;
} tokenizer_t;


/**
 * create a new tokenizer
 * @return pointer to the new tokenizer
 */
tokenizer_t *tokenizer_new();

/**
 * init all the fields of the tokenizer
 * @param tz the tokenizer
 */
void tokenizer_init(tokenizer_t *tz, char *input);

/**
 * free the tokenizer and its resources
 * @param tz pointer to the tokenizer to free
 */
void tokenizer_free(tokenizer_t *tz);

/**
 * free the token's value if it is non null
 * @param tok pointer to the token to free
 */
void tokenizer_free_token(token_t *tok);

/**
 * get the next token from the input
 * @param tz pointer to the tokenizer
 * @return the next token
 */
token_t tokenizer_next_token(tokenizer_t *tz);

/**
 * print the token type and value for debugging
 * @param tok the token to print
 * @param buf buffer to write the string representation limited to buf_sz
 * @param buf_sz size of the buffer
 * @return number of characters that would have been written
 */
size_t tokenizer_token_str(token_t tok, char *buf, size_t buf_sz);


#endif // __TOKENIZER_H__