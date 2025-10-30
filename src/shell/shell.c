/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "shell.h"
#include <signal.h>
#include <string.h>
#include <errno.h>


static lexer_t *lex;


int shell_init() {

    // Initialize shell state early so signal handlers can safely access it.
    shell_state_init();

    // create lexer after state init
    lex = lexer_new();

    // Only try to take terminal control if stdin is a TTY
    if (isatty(STDIN_FILENO)) {
        // Put shell in its own process group
        pid_t shell_pid = getpid();
        if (setpgid(shell_pid, shell_pid) == -1) {
            // ignore/setpgid failure if already in pgid
        }

        // Give terminal control to the shell's process group. Essential for job control.
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1 && errno != ENOTTY) {
            perror("tcsetpgrp failed");
            return 1;
        }
    } else {
        fprintf(stderr, "warning: stdin is not a TTY, job control disabled\n");
    }

    // Disable buffering for stdout. Ensures immediate output for status messages.
    setbuf(stdout, NULL);
    // rl_event_hook = signal_handling_hook;
    using_history();
    return 0;
}

void shell_cleanup() {
    history_trim();
    lexer_free(lex);
    shell_state_free();
}

int shell_loop() {
    shell_state_t *shell_state = shell_state_get();

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
            if (shell_state->running_jobs_count > 0 && !warning_exit) {
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
        history_save_command(lex->input);
        exec_node(ast_node);
        parser_free_ast(ast_node);
        free(input);
        
    } while (!shell_state->should_exit);
    
    return 0;
}