/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __STATE_H__
#define __STATE_H__

#include "config.h"
#include "utils/collections.h"
#include "utils/system/syscall.h"
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

// Forward declarations to avoid circular dependencies
typedef struct job_t job_t;
typedef struct history_t history_t;

typedef struct {
  char *key;
  char *value;
} env_var_t;

typedef struct {
    char *command;
    int exit_status;
    pid_t bg_pid;
    pid_t pgid;
    double duration_ms;
    struct timespec started_at;
    struct timespec ended_at;
} shell_last_exec_t;

typedef struct {
  bool interactive;
  bool job_control;
  bool history_enabled;
  bool debug;
} shell_flags_t;

typedef struct {
    char hostname[256];  
    char username[256];  
    uid_t uid;          
    gid_t gid;           
    pid_t pid;           
    pid_t pgid;          
    char *cwd;
} shell_identity_t;

typedef struct {
    job_t *jobs;
    job_t *jobs_tail;
    size_t jobs_count;
    size_t running_jobs_count;
} shell_jobs_t;

/**
 * @brief Main structure holding the global state of the Novash shell.
 * This singleton structure is accessed throughout the program via
 * shell_state_get().
 */
typedef struct shell_state_t {
  shell_identity_t identity;
  env_var_t *environment;

  shell_last_exec_t last_exec;
  
  history_t *hist;
  shell_jobs_t jobs;
  
  shell_flags_t flags;
  struct termios shell_tmodes;
  bool should_exit;

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
 * @brief Retrieves the value of an environment variable from the internal
 * hashmap.
 * @param key The environment variable name (e.g., "PATH").
 * @return char* The value string if found, or NULL if not found.
 */
char *shell_state_getenv(const char *key);


shell_identity_t *shell_state_get_identity();
shell_jobs_t *shell_state_get_jobs();
shell_last_exec_t *shell_state_get_last_exec();
char *shell_state_get_flags(void);

/**
 * @brief Regains control of the terminal for the shell process.
 * This is typically called after a foreground job has completed or stopped.
 */
void shell_regain_control();
/**
 * @brief Frees all dynamically allocated resources within the shell state.
 * This includes the environment hashmap, history, jobs, and cwd.
 */
void shell_state_free();

void shell_reset_last_exec();

#endif // __STATE_H__
