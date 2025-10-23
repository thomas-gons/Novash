// tests/test_parser.c
#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "tokenizer/tokenizer.h"
#include "parser/parser.h"
#include "utils/memory.h" // si tu as xmalloc/xstrdup prototypes

Test(parser, sequence_and_background_flags) {
    tokenizer_t *tz = tokenizer_new();
    tokenizer_init(tz, "sleep 5 & sleep 4 & sleep 3");

    ast_node_t *root = parser_create_ast(tz);
    cr_assert_not_null(root);
    cr_assert_eq(root->type, NODE_SEQUENCE);

    size_t n = arr_len(root->seq.nodes);
    cr_expect_eq(n, 3, "expected 3 commands in sequence, got %zu", n);

    // first two must be background
    cr_expect_eq(root->seq.nodes[0]->type, NODE_CMD);
    cr_expect(root->seq.nodes[0]->cmd.is_bg, "first command should be BG");
    cr_expect_str_eq(root->seq.nodes[0]->cmd.argv[0], "sleep");

    cr_expect(root->seq.nodes[1]->cmd.is_bg, "second command should be BG");
    cr_expect_not(root->seq.nodes[2]->cmd.is_bg, "third command should be FG");

    parser_free_ast(root);
    tokenizer_free(tz);
    tokenizer_free(tz); // if your tokenizer_free exists; otherwise adjust
}