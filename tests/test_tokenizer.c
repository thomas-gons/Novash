#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include "tokenizer/tokenizer.h"

Test(tokenizer, complex_quotes_and_metachars) {
    tokenizer_t *tz = tokenizer_new();
    
    char *input = "echo \"Hello 'world'\" 'and \"friends\"' > output.txt 2>> log.txt &";
    tokenizer_init(tz, input);

    token_t tok;

    // echo
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "echo");
    tokenizer_free_token(&tok);

    // "Hello 'world'"
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "Hello 'world'");
    tokenizer_free_token(&tok);

    // 'and "friends"'
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "and \"friends\"");
    tokenizer_free_token(&tok);

    // >
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);

    // output.txt
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "output.txt");
    tokenizer_free_token(&tok);

    // 2
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_FD);

    // >>
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    
    // log.txt
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "log.txt");
    tokenizer_free_token(&tok);

    // &
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_BG);

    // EOF
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_EOF);

    tokenizer_free(tz);
}

Test(tokenizer, edge_cases) {
    tokenizer_t *tz = tokenizer_new();

    // empty input
    tokenizer_init(tz, "");
    token_t tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_EOF);

    // only metacharacters
    tokenizer_init(tz, "|| && ; > >> < &");
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_OR);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_AND);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_SEMI);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_REDIR_OUT);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_REDIR_APPEND);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_REDIR_IN);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_BG);
    tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_EOF);

    tokenizer_free(tz);
}
