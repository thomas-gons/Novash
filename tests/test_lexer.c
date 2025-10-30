/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include "lexer/lexer.h"

Test(lexer, complex_quotes_and_metachars) {
    lexer_t *lex = lexer_new();
    
    char *input = "echo \"Hello 'world'\" 'and \"friends\"' > output.txt 2>> log.txt &";
    lexer_init(lex, input);

    token_t tok;

    // echo
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "echo");
    lexer_free_token(&tok);

    // "Hello 'world'"
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "Hello 'world'");
    lexer_free_token(&tok);

    // 'and "friends"'
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "and \"friends\"");
    lexer_free_token(&tok);

    // >
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);

    // output.txt
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "output.txt");
    lexer_free_token(&tok);

    // 2
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_FD);

    // >>
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    
    // log.txt
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "log.txt");
    lexer_free_token(&tok);

    // &
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_BG);

    // EOF
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_EOF);

    lexer_free(lex);
}

Test(lexer, edge_cases) {
    lexer_t *lex = lexer_new();

    // empty input
    lexer_init(lex, "");
    token_t tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_EOF);

    // only metacharacters
    lexer_init(lex, "|| && ; > >> < &");
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_OR);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_AND);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_SEMI);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_IN);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_BG);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_EOF);

    lexer_free(lex);
}
