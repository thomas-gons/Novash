/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "shell.h"


static tokenizer_t *tz;

int shell_init() {
    // Ignore SIGTTOU and SIGTTIN signals: standard practice for job control
    // so background jobs don't stop when trying to read/write to the terminal.
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    // Set signal handlers to set atomic flags instead of performing logic
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    // Give terminal control to the shell's process group. Essential for job control.
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
        perror("tcsetpgrp failed");
        return 1;
    }
    shell_state_init();
    tz = tokenizer_new();

    // Disable buffering for stdout. Ensures immediate output for status messages.
    setbuf(stdout, NULL);

    return 0;
}

void shell_cleanup() {
    history_trim();
    tokenizer_free(tz);
    shell_state_free();
}

int shell_run() {
    shell_state_t *shell_state = shell_state_get();

    char *input = NULL;

    // Flag to track if an exit warning for running jobs has been given
    bool warning_exit = false;
    do {
        // --- Signal Handling Block ---
        // Check atomic flags set by signal handlers and perform necessary synchronous actions.
        if (shell_state->sigchld_received) {
            shell_state->sigchld_received = false;
            handle_sigchld_events();
        }
        if (shell_state->sigint_received) {
            shell_state->sigint_received = false;
            handle_sigint_event();
            // Reset exit warning after an interruption (Ctrl+C)
            warning_exit = false; 
        }
        // --- End Signal Handling Block ---

        input = readline("$ ");
        if (input == NULL) {
            // If running jobs exist, show a warning on first Ctrl+D
            if (shell_state->running_jobs_count > 0 && !warning_exit) {
                printf("you have running jobs\n");
                warning_exit = true;
                continue;  // Continue the loop, don't exit yet
            } else {
                // Exit if no running jobs or if confirmed (second Ctrl+D)
                printf("exit\n");
                break;
            }
        }

        warning_exit = false;
        tokenizer_init(tz, input);
        
        ast_node_t *ast_node = parser_create_ast(tz);
        history_save_command(tz->input);

        exec_node(ast_node);
        parser_free_ast(ast_node);
        free(input);
        
    } while (!shell_state->should_exit);
    
    return 0;
}