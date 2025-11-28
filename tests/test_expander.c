/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#include "expander/expander.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "shell/shell.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>

static ast_node_t *parse_input(const char *input) {
  // ensure shell is initialized for expansions
  shell_init(true);

  lexer_t *lex = lexer_new();
  lexer_init(lex, (char *)input);
  ast_node_t *ast = parser_create_ast(lex);
  expander_expand_ast(ast);
  return ast;
}

Test(expander, variable) {
  const char *input = "$VAR $? $! $- $$";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 1);
  cmd_node_t cmd = ast->seq.nodes[0]->cmd;
  cr_assert_eq(arrlen(cmd.argv), 5);
}

Test(expander, tilde) {
  // if ~user exists, this test may fail peek a non-existing user instead
  const char *input = "~ ~root; ~user";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 2);

  cmd_node_t cmd = ast->seq.nodes[0]->cmd;
  cr_assert_eq(arrlen(cmd.argv), 2);
  cr_assert_str_eq(cmd.argv[0], getenv("HOME"));
  cr_assert_str_eq(cmd.argv[1], "/root");

  cmd_node_t cmd2 = ast->seq.nodes[1]->cmd;
  cr_assert_eq(cmd2.argv, NULL);
  cr_assert_eq(ast->invalid, true);
}

Test(expander, globbing) {
  const char *input = "tests/*.c;";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 1);

  cmd_node_t cmd = ast->seq.nodes[0]->cmd;
  int real_count = 0;
  glob_t globbuf;
  char pattern[PATH_MAX];
  snprintf(pattern, sizeof(pattern), "tests/*.c");
  if (glob(pattern, 0, NULL, &globbuf) == 0) {
    real_count = globbuf.gl_pathc;
    globfree(&globbuf);
  }

  cr_assert_eq(arrlen(cmd.argv), real_count);
}

Test(expander, invalid_globbing) {
  const char *input = "tests/*.ctest;";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 1);

  cmd_node_t cmd = ast->seq.nodes[0]->cmd;
  cr_assert_eq(ast->invalid, true);
  cr_assert_eq(cmd.argv, NULL);
}