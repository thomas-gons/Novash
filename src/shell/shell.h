/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef __SHELL_H__
#define __SHELL_H__

#include "executor/executor.h"
#include "executor/jobs.h"
#include "history/history.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "expander/expander.h"
#include "shell/signal.h"
#include "shell/state.h"
#include "prompt/ps1.h"
#include <errno.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


/**
 * @brief Initializes all necessary shell subsystems.
 * * Sets up signal handlers (to ignore SIGTTOU/SIGTTIN and handle
 * SIGINT/SIGCHLD), establishes terminal control (tcsetpgrp), and initializes
 * global state, and output buffering.
 * * @return 0 on successful initialization, 1 on failure (e.g., terminal
 * control error).
 */
int shell_init();

/**
 * @brief Executes the main Read-Eval-Print Loop (REPL) of the shell.
 * * This function handles user input via Readline, processes signal flags set
 * asynchronously, parses the command line, and executes the resulting AST.
 * The loop continues until the 'exit' command is executed or the user confirms
 * exit while running background jobs.
 * * @return The final exit status of the shell process (0 on clean exit).
 */
int shell_loop();

/**
 * @brief Performs necessary cleanup before the shell terminates.
 * * This includes trimming and saving command history
 * and releasing the global shell state memory.
 */
void shell_cleanup();

#endif // __SHELL_H__