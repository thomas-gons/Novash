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

Test(lexer, literal_word_part) {
    lexer_t *lex = lexer_new();
    lexer_init(lex, "word /path/to/file.ext \"mixed 'quotes' here\" \\tescaped");
    token_t tok;

    // word -> word [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "word");
    lexer_free_token(&tok);

    // /path/to/file.ext -> /path/to/file.ext [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "/path/to/file.ext");
    lexer_free_token(&tok);

    // "mixed 'quotes' here" -> mixed 'quotes' here [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "mixed 'quotes' here");
    lexer_free_token(&tok);

    // \escaped -> escaped [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "\tescaped");
    lexer_free_token(&tok);

    lexer_free(lex);
}

Test(lexer, variable_word_part) {
    lexer_t *lex = lexer_new();
    lexer_init(lex, "$VAR_NAME ${VAR_NAME} $? ${?} $ ${}abc");
    token_t tok;

    // $VAR_NAME -> VAR_NAME [VAR]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_VARIABLE);
    cr_assert_str_eq(tok.parts[0].value, "VAR_NAME");
    lexer_free_token(&tok);

    // ${VAR_NAME} -> VAR_NAME [VAR]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_VARIABLE);
    cr_assert_str_eq(tok.parts[0].value, "VAR_NAME");
    lexer_free_token(&tok);

    // $? -> ? [VAR]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_VARIABLE);
    cr_assert_str_eq(tok.parts[0].value, "?");
    lexer_free_token(&tok);

    // ${?} -> ? [VAR]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_VARIABLE);
    cr_assert_str_eq(tok.parts[0].value, "?");
    lexer_free_token(&tok);

    // $ -> $ [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "$");
    lexer_free_token(&tok);

    // ${}abc -> abc [LIT] (ignore empty var)
    tok = lexer_next_token(lex);
    cr_assert_eq(arrlen(tok.parts), 1);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "abc");
    lexer_free_token(&tok);

    lexer_free(lex);
}

Test(lexer, tilde_word_part) {
    lexer_t *lex = lexer_new();
    lexer_init(lex, "~ ~user ~/path \\~~");
    token_t tok;

    // ~ -> ~ [TILDE]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_TILDE);
    cr_assert_str_eq(tok.parts[0].value, "~");
    lexer_free_token(&tok);

    // ~user -> ~user [TILDE]
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.parts[0].type, WORD_TILDE);
    cr_assert_str_eq(tok.parts[0].value, "~user");
    lexer_free_token(&tok);

    // ~/path -> ~ [TILDE] and /path [LIT]
    tok = lexer_next_token(lex);
    cr_assert_eq(arrlen(tok.parts), 2);
    cr_assert_eq(tok.parts[0].type, WORD_TILDE);
    cr_assert_str_eq(tok.parts[0].value, "~");
    lexer_free_token(&tok);

    // \~~ -> ~ [LIT] and ~ [TILDE]
    tok = lexer_next_token(lex);
    cr_assert_eq(arrlen(tok.parts), 2);
    cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
    cr_assert_str_eq(tok.parts[0].value, "~");
    cr_assert_eq(tok.parts[1].type, WORD_TILDE);
    cr_assert_str_eq(tok.parts[1].value, "~");
    lexer_free_token(&tok);
}

// Test(lexer, globbing_word_part) {
//     lexer_t *lex = lexer_new();
//     lexer_init(lex, "file*name?.txt [a-z]* {brace} \\*literal\\?");
//     token_t tok;

//     // file*name?.txt -> file, *, name, ?, .txt
//     tok = lexer_next_token(lex);
//     cr_assert_eq(tok.type, TOK_WORD);
//     cr_assert_eq(arrlen(tok.parts), 5);

//     cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
//     cr_assert_str_eq(tok.parts[0].value, "file");
//     cr_assert_eq(tok.parts[1].type, WORD_GLOB);
//     cr_assert_str_eq(tok.parts[1].value, "*");
//     cr_assert_eq(tok.parts[2].type, WORD_LITERAL);
//     cr_assert_str_eq(tok.parts[2].value, "name");
//     cr_assert_eq(tok.parts[3].type, WORD_GLOB);
//     cr_assert_str_eq(tok.parts[3].value, "?");
//     cr_assert_eq(tok.parts[4].type, WORD_LITERAL);
//     cr_assert_str_eq(tok.parts[4].value, ".txt");
//     lexer_free_token(&tok);

//     // [a-z]* -> [a-z], *
//     tok = lexer_next_token(lex);
//     cr_assert_eq(arrlen(tok.parts), 2);
//     cr_assert_eq(tok.parts[0].type, WORD_GLOB);
//     cr_assert_str_eq(tok.parts[0].value, "[a-z]");
//     cr_assert_eq(tok.parts[1].type, WORD_GLOB);
//     cr_assert_str_eq(tok.parts[1].value, "*");
//     lexer_free_token(&tok);

//     // {brace} -> {brace}
//     tok = lexer_next_token(lex);
//     cr_assert_eq(arrlen(tok.parts), 1);
//     cr_assert_eq(tok.parts[0].type, WORD_GLOB);
//     cr_assert_str_eq(tok.parts[0].value, "{brace}");
//     lexer_free_token(&tok);

//     // \*literal\? -> *literal?
//     tok = lexer_next_token(lex);
//     cr_assert_eq(arrlen(tok.parts), 1);
//     cr_assert_eq(tok.parts[0].type, WORD_LITERAL);
//     cr_assert_str_eq(tok.parts[0].value, "*literal?");
//     lexer_free_token(&tok);

//     lexer_free(lex);
// }

Test(lexer, basic_tokens) {
    lexer_t *lex = lexer_new();

    // empty input
    lexer_init(lex, "");
    token_t tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_EOF);

    // only metacharacters
    lexer_init(lex, "> >> < | || && ; &");

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_IN);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_PIPE);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_OR);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_AND);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_SEMI);
    
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_BG);
    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_EOF);

    lexer_free(lex);
}

Test(lexer, fd_token) {
    lexer_t *lex = lexer_new();

    // fd before redirection
    lexer_init(lex, "2> output.txt 10>> append.log");
    token_t tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_FD);
    cr_assert_str_eq(tok.raw_value, "2");
    lexer_free_token(&tok);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);
    lexer_free_token(&tok);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.raw_value, "output.txt");
    lexer_free_token(&tok);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_FD);
    cr_assert_str_eq(tok.raw_value, "10");
    lexer_free_token(&tok);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    lexer_free_token(&tok);

    tok = lexer_next_token(lex);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.raw_value, "append.log");
    lexer_free_token(&tok);

    lexer_free(lex);
}
