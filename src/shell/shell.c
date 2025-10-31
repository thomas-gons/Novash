/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "shell.h"
#include <errno.h>
#include <signal.h>
#include <string.h>

static lexer_t *lex;

static int shell_event_hook() {
  handle_sigchld_events();
  // Placeholder for future event handling (e.g., signal processing)
  return 0;
}

int shell_init() {
  // Initialize shell state early so signal handlers can safely access it.
  shell_state_init();
  builtin_init();
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);

  // create lexer after state init
  lex = lexer_new();
  // Only try to take terminal control if stdin is a TTY
  if (isatty(STDIN_FILENO)) {
    // Put shell in its own process group
    if (setpgid(0, 0) == -1) {
      if (errno != EACCES && errno != EINVAL && errno != EPERM) {
        perror("setpgid failed");
        return 1;
      }
    }

    shell_state_t *sh_state = shell_state_get();
    sh_state->pgid = getpgrp();
    xtcsetpgrp(STDIN_FILENO, sh_state->pgid);

    sh_state->shell_tmodes = (struct termios){0};
    xtcgetattr(STDIN_FILENO, &sh_state->shell_tmodes);
    
  } else {
    fprintf(stderr, "warning: stdin is not a TTY, job control disabled\n");
  }

  // Disable buffering for stdout. Ensures immediate output for status messages.
  setbuf(stdout, NULL);
  rl_event_hook = shell_event_hook;
  using_history();
  return 0;
}

void shell_cleanup() {
  history_trim();
  lexer_free(lex);
  shell_state_free();
}

int shell_loop() {
  shell_state_t *sh_state = shell_state_get();
  char *input = NULL;

  // Flag to track if an exit warning for running jobs has been given
  bool warning_exit = false;
  do {
    errno = 0;
    input = readline("$ ");
    if (!input) {
      if (errno == EINTR) {
        // readline was interrupted by a signal, go back to loop immediately
        continue;
      }

      // EOF (Ctrl+D)
      if (sh_state->running_jobs_count > 0 && !warning_exit) {
        printf("you have running jobs\n");
        warning_exit = true;
        continue;
      } else {
        printf("exit\n");
        break;
      }
    }

    warning_exit = false;
    lexer_init(lex, input);

    ast_node_t *ast_node = parser_create_ast(lex);
    char *ast_str = parser_ast_str(ast_node, 0);
    pr_debug("AST:\n%s", ast_str);
    free(ast_str);
    history_save_command(lex->input);
    exec_node(ast_node);
    parser_free_ast(ast_node);
    free(input);
  } while (!sh_state->should_exit);

  return 0;
}