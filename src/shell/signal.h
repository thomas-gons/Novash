/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "executor/jobs.h"
#include "shell/state.h"
#include "utils/log.h"
#include "utils/system/syscall.h"
#include <errno.h>
#include <readline/readline.h>
#include <stdio.h>

/**
 * @brief Synchronously handles the SIGINT signal event.
 * flag is set. Its main responsibility is to clean up the Readline interface
 * (clearing the current line and redisplaying the prompt) after a Ctrl+C.
 */
void handle_sigint_event();

/**
 * @brief Synchronously processes all pending SIGCHLD events.
 * * This function is called from the main shell loop when the sigchld_received
 * flag is set. It uses waitpid() in a non-blocking loop to clean up terminated
 * or stopped child processes and updates the shell's job list and display.
 */
void handle_sigchld_events();

#endif // __SIGNAL_H__