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

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <readline/readline.h>
#include "shell/state.h"
#include "shell/signal.h"
#include "executor/jobs.h"
#include "history/history.h"
#include "tokenizer/tokenizer.h"
#include "parser/parser.h"
#include "executor/executor.h"

/**
 * @brief Initializes all necessary shell subsystems.
 * * Sets up signal handlers (to ignore SIGTTOU/SIGTTIN and handle SIGINT/SIGCHLD),
 * establishes terminal control (tcsetpgrp), and initializes global state, 
 * and output buffering.
 * * @return 0 on successful initialization, 1 on failure (e.g., terminal control error).
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
int shell_run();

/**
 * @brief Performs necessary cleanup before the shell terminates.
 * * This includes trimming and saving command history 
 * and releasing the global shell state memory.
 */
void shell_cleanup();

#endif // __SHELL_H__