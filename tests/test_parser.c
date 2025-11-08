/*
 * tests_parser.c — Tests pour le parser Novash
 * Copyright (C) 2025 Thomas Gons
 *
 * Licence: GNU GPL v3 ou ultérieure
 */

 #define _GNU_SOURCE

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include "lexer/lexer.h"
#include "parser/parser.h"

static char* ast_to_string(ast_node_t *node) {
    // This function uses a static buffer to capture the print_ast output
    // Warning: limited to 16k chars for simplicity
    static char buf[16384];
    memset(buf, 0, sizeof(buf));
    FILE *memstream = fmemopen(buf, sizeof(buf), "w");
    if (!memstream) return NULL;

    // Redirect stdout temporarily
    FILE *old = stdout;
    stdout = memstream;
    print_ast(node, 0);
    fflush(memstream);
    stdout = old;
    fclose(memstream);
    return buf;
}

static ast_node_t* parse_input(const char *input) {
    tokenizer_t *tz = tokenizer_new();
    tokenizer_init(tz, (char*)input);
    ast_node_t *ast = parser_create_ast(tz);
    tokenizer_free(tz);
    return ast;
}

Test(parser, pipeline_with_redirection) {
    const char *input = "echo hello | grep h > out.txt";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert_not_null(ast);
    cr_assert(strstr(str, "PIPELINE"));
    cr_assert(strstr(str, "CMD: echo hello"));
    cr_assert(strstr(str, "CMD: grep h [1>out.txt]"));

    parser_free_ast(ast);
}

Test(parser, background_sequence) {
    const char *input = "sleep 1 & ls -l";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert(strstr(str, "SEQUENCE"));
    cr_assert(strstr(str, "CMD: sleep 1 &"));
    cr_assert(strstr(str, "CMD: ls -l"));

    parser_free_ast(ast);
}

Test(parser, conditional_and_or) {
    const char *input = "make && echo done || echo fail";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert(strstr(str, "CONDITIONAL (&&)"));
    cr_assert(strstr(str, "CONDITIONAL (||)"));
    cr_assert(strstr(str, "CMD: make"));
    cr_assert(strstr(str, "CMD: echo done"));
    cr_assert(strstr(str, "CMD: echo fail"));

    parser_free_ast(ast);
}

Test(parser, complex_combination) {
    const char *input = "cmd1 arg | cmd2 arg && cmd3 > f.txt; cmd4 arg5 &";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert(strstr(str, "SEQUENCE"));
    cr_assert(strstr(str, "PIPELINE"));
    cr_assert(strstr(str, "CONDITIONAL (&&)"));
    cr_assert(strstr(str, "CMD: cmd3 [1>f.txt]"));
    cr_assert(strstr(str, "CMD: cmd4 arg5 &"));

    parser_free_ast(ast);
}

Test(parser, fd_redirections) {
    const char *input = "echo hello 1>> out.log 2> err.log";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert(strstr(str, "CMD: echo hello [1>>out.log, 2>err.log]"));

    parser_free_ast(ast);
}

Test(parser, quotes_and_special_chars) {
    const char *input = "echo \"hello | world\" 'test && fail' | grep world";
    ast_node_t *ast = parse_input(input);
    char *str = ast_to_string(ast);

    cr_assert(strstr(str, "PIPELINE"));
    cr_assert(strstr(str, "CMD: echo hello | world test && fail"));
    cr_assert(strstr(str, "CMD: grep world"));

    parser_free_ast(ast);
}
