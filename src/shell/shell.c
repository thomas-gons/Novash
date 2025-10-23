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


static tokenizer_t *tz;

// Check atomic flags set by signal handlers and perform necessary synchronous actions.
static int signal_handling_hook(void) {
    shell_state_t *shell_state = shell_state_get();

    if (shell_state->sigchld_received) {
        printf("Handling SIGCHLD\n");
        shell_state->sigchld_received = false;
        handle_sigchld_events();
        rl_on_new_line();
        rl_redisplay();
    }

    if (shell_state->sigint_received) {
        shell_state->sigint_received = false;
        handle_sigint_event();
    }

    return 0; // continue readline
}

int shell_init() {
    // Ignore SIGTTOU and SIGTTIN signals: standard practice for job control
    // so background jobs don't stop when trying to read/write to the terminal.
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    
    // The shell should ignore terminal stop signals itself (so it is not stopped).
    // Child processes will still receive SIGTSTP when user presses Ctrl+Z.
    signal(SIGTSTP, SIG_IGN);

    // Initialize shell state early so signal handlers can safely access it.
    shell_state_init();

    // create tokenizer after state init
    tz = tokenizer_new();

    // Register signal handlers using sigaction for more predictable behavior.
    struct sigaction sa;

    // SIGINT: set handler, allow restart of syscalls (optional)
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigint_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction(SIGINT)");
        return 1;
    }

    // SIGCHLD: set handler, DO NOT set SA_RESTART so that blocking readline is interrupted
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = 0; // important: no SA_RESTART, we want readline to be interrupted
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction(SIGCHLD)");
        return 1;
    }

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
    rl_event_hook = signal_handling_hook;
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
        if (shell_state->sigchld_received) {
            shell_state->sigchld_received = false;
            handle_sigchld_events();
            rl_on_new_line();
            rl_redisplay();
        }
        if (shell_state->sigint_received) {
            shell_state->sigint_received = false;
            handle_sigint_event();
        }

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
        tokenizer_init(tz, input);
        
        ast_node_t *ast_node = parser_create_ast(tz);
        history_save_command(tz->input);

        exec_node(ast_node);
        parser_free_ast(ast_node);
        free(input);
        
    } while (!shell_state->should_exit);
    
    return 0;
}