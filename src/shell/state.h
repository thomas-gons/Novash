/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __STATE_H__
#define __STATE_H__

#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "config.h"
#include "utils/collections.h"


// Forward declarations to avoid circular dependencies
typedef struct job_t job_t;
typedef struct history_t history_t;

/**
 * @brief Main structure holding the global state of the Novash shell.
 * This singleton structure is accessed throughout the program via shell_state_get().
 */
typedef struct shell_state_t{
    hashmap_t *environment;

    char *cwd;
    int last_exit_status;
    bool should_exit;

    history_t *hist;
    job_t *jobs;
    size_t jobs_count;
    size_t running_jobs_count;

    // --- Signal State (Accessed by signal handlers) ---
    volatile sig_atomic_t sigint_received;
    volatile sig_atomic_t sigchld_received;
    volatile sig_atomic_t sigquit_received;
} shell_state_t;

/**
 * @brief Allocates and initializes the global shell state (the singleton).
 * Must be called once at the start of the program.
 */
void shell_state_init();

/**
 * @brief Retrieves the pointer to the initialized global shell state.
 * Performs a sanity check and may exit if called before initialization.
 * @return shell_state_t* The global shell state pointer.
 */
shell_state_t *shell_state_get();

/**
 * @brief Retrieves the value of an environment variable from the internal hashmap.
 * @param key The environment variable name (e.g., "PATH").
 * @return char* The value string if found, or NULL if not found.
 */
char *shell_state_getenv(const char *key);

/**
 * @brief Frees all dynamically allocated resources within the shell state.
 * This includes the environment hashmap, history, jobs, and cwd.
 */
void shell_state_free();

#endif // __STATE_H__
