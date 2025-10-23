// tests/test_tokenizer.c
#include <criterion/criterion.h>
#include "tokenizer/tokenizer.h"

Test(tokenizer, basic_tokens) {
    tokenizer_t *tz = tokenizer_new();
    tokenizer_init(tz, "echo foo && ls | grep x; sleep 1 &");

    token_t tok = tokenizer_next_token(tz);
    cr_assert_eq(tok.type, TOK_WORD);
    cr_assert_str_eq(tok.value, "echo");

    // consume until && and assert types
    while ((tok = tokenizer_next_token(tz)).type != TOK_AND) {
        /* skip */
    }
    cr_assert_eq(tok.type, TOK_AND);

    // further checks left as exercise...
    tokenizer_free(tz);
}