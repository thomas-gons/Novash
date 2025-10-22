/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <stdio.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include "shell/state.h"
#include "executor/jobs.h"
#include <readline/readline.h>


/**
 * @brief Synchronously processes all pending SIGCHLD events.
 * * This function is called from the main shell loop when the sigchld_received 
 * flag is set. It uses waitpid() in a non-blocking loop to clean up terminated 
 * or stopped child processes and updates the shell's job list and display.
 */
void handle_sigchld_events();

/**
 * @brief Synchronously handles the SIGINT signal event.
 * flag is set. Its main responsibility is to clean up the Readline interface 
 * (clearing the current line and redisplaying the prompt) after a Ctrl+C.
 */
void handle_sigint_event();

/**
 * @brief Asynchronous signal handler for SIGINT (Ctrl+C).
 * * This function must be minimal and only sets the atomic sigint_received flag 
 * in the shell state for later synchronous processing.
 * * @param sig The signal number (SIGINT).
 */
void sigint_handler(int sig);

/**
 * @brief Asynchronous signal handler for SIGCHLD (Child status change).
 * * This function must be minimal and only sets the atomic sigchld_received flag 
 * in the shell state for later synchronous processing.
 * * @param sig The signal number (SIGCHLD).
 */
void sigchld_handler(int sig);

#endif // __SIGNAL_H__