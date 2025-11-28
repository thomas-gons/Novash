/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#define _GNU_SOURCE

#include "lexer/lexer.h"
#include "parser/parser.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>

static ast_node_t *parse_input(const char *input) {
  lexer_t *lex = lexer_new();
  lexer_init(lex, (char *)input);
  ast_node_t *ast = parser_create_ast(lex);
  lexer_free(lex);
  return ast;
}

Test(parser, pipeline_with_redirection) {
  const char *input = "echo hello | grep h > out.txt";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 1);

  pipe_node_t pipe = ast->seq.nodes[0]->pipe;
  cr_assert_eq(arrlen(pipe.nodes), 2);

  cmd_node_t first_cmd = pipe.nodes[0]->cmd;
  cr_assert_eq(arrlen(first_cmd.argv_parts), 2);

  cmd_node_t second_cmd = pipe.nodes[1]->cmd;
  cr_assert_eq(arrlen(second_cmd.argv_parts), 2);
  cr_assert_eq(second_cmd.redir->type, REDIR_OUT);
  word_part_t *redir_target_parts = second_cmd.redir->target_parts;
  cr_assert_eq(arrlen(redir_target_parts), 1);
  cr_assert_eq(redir_target_parts[0].type, WORD_LITERAL);
  cr_assert_str_eq(redir_target_parts[0].value, "out.txt");

  parser_free_ast(ast);
}

Test(parser, background_sequence) {
  const char *input = "sleep 1 & ls -l";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 2);

  // sleep 1 &
  cmd_node_t first_cmd = ast->seq.nodes[0]->cmd;
  cr_assert_eq(arrlen(first_cmd.argv_parts), 2);
  cr_assert(first_cmd.is_bg);

  // ls -l
  cmd_node_t second_cmd = ast->seq.nodes[1]->cmd;
  cr_assert_eq(arrlen(second_cmd.argv_parts), 2);
  cr_assert(!second_cmd.is_bg);
  parser_free_ast(ast);
}

Test(parser, conditional_and_or) {
  const char *input = "make && echo done || echo fail";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 1);

  // primary condition is OR (make && echo done) || echo fail
  cond_node_t cond = ast->seq.nodes[0]->cond;
  cr_assert_eq(cond.op, COND_OR);
  cr_assert_eq(cond.left->type, NODE_CONDITIONAL);
  cr_assert_eq(cond.right->type, NODE_CMD);

  // sub-condition is AND make && echo done
  cond_node_t left_cond = cond.left->cond;
  cr_assert_eq(left_cond.op, COND_AND);
  cr_assert_eq(left_cond.left->type, NODE_CMD);
  cr_assert_eq(left_cond.right->type, NODE_CMD);
  parser_free_ast(ast);
}

Test(parser, complex_combination) {
  const char *input = "cmd1 arg | cmd2 arg && cmd3 > f.txt; cmd4 arg5 &";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);
  cr_assert_eq(arrlen(ast->seq.nodes), 2);

  // First sequence node -> cmd1 arg | cmd2 arg && cmd3 > f.txt
  cond_node_t cond = ast->seq.nodes[0]->cond;
  cr_assert_eq(cond.op, COND_AND);

  cr_assert_eq(cond.left->type, NODE_PIPELINE);
  cr_assert_eq(cond.right->type, NODE_CMD);

  // Pipeline node (left condition side) -> cmd1 arg | cmd2 arg
  pipe_node_t pipe = cond.left->pipe;
  cr_assert_eq(arrlen(pipe.nodes), 2);

  // Command node (right condition side) -> cmd3 > f.txt
  cmd_node_t cmd3 = cond.right->cmd;
  cr_assert_eq(arrlen(cmd3.argv_parts), 1);
  cr_assert_eq(cmd3.redir->type, REDIR_OUT);
  word_part_t *redir_target_parts = cmd3.redir->target_parts;
  cr_assert_eq(arrlen(redir_target_parts), 1);
  cr_assert_eq(redir_target_parts[0].type, WORD_LITERAL);
  cr_assert_str_eq(redir_target_parts[0].value, "f.txt");

  // Second sequence node -> cmd4 arg5 &
  cmd_node_t cmd4 = ast->seq.nodes[1]->cmd;
  cr_assert_eq(arrlen(cmd4.argv_parts), 2);
  cr_assert(cmd4.is_bg);

  parser_free_ast(ast);
}

Test(parser, fd_redirections) {
  const char *input = "echo hello 1>> out.log 2> err.log";
  ast_node_t *ast = parse_input(input);
  cr_assert_not_null(ast);
  cr_assert_eq(ast->type, NODE_SEQUENCE);

  cmd_node_t cmd = ast->seq.nodes[0]->cmd;
  cr_assert_eq(arrlen(cmd.argv_parts), 2);
  cr_assert_eq(arrlen(cmd.redir), 2);
  // 1>> out.log
  redirection_t *redir1 = cmd.redir;
  cr_assert_eq(redir1->fd, 1);
  cr_assert_eq(redir1->type, REDIR_APPEND);
  word_part_t *redir1_target_parts = redir1->target_parts;
  cr_assert_eq(arrlen(redir1_target_parts), 1);
  cr_assert_eq(redir1_target_parts[0].type, WORD_LITERAL);
  cr_assert_str_eq(redir1_target_parts[0].value, "out.log");

  // 2> err.log
  redirection_t *redir2 = &cmd.redir[1];
  cr_assert_eq(redir2->fd, 2);
  cr_assert_eq(redir2->type, REDIR_OUT);
  word_part_t *redir2_target_parts = redir2->target_parts;
  cr_assert_eq(arrlen(redir2_target_parts), 1);
  cr_assert_eq(redir2_target_parts[0].type, WORD_LITERAL);
  cr_assert_str_eq(redir2_target_parts[0].value, "err.log");
  parser_free_ast(ast);
}
