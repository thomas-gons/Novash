/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __BUILTIN_H__
#define __BUILTIN_H__

#include <stdbool.h>
#include "shell/state.h"
#include "utils/utils.h"
#include "history/history.h"
#include "executor/jobs.h"


/**
 * @brief Function pointer type for all internal (builtin) commands.
 * Builtins receive the argument count (argc) and the argument vector (argv).
 * @param argc The number of arguments (including the command name).
 * @param argv The array of argument strings.
 * @return int The exit status of the command (0 for success).
 */
typedef int (*builtin_f_t) (int argc, char *argv[]);

/**
 * @brief Structure mapping a command name to its corresponding function.
 */
typedef struct {
    char *name;
    builtin_f_t f;
} builtin_t;


/**
 * @brief Checks if a given command name corresponds to an internal shell builtin.
 * @param name The command name string.
 * @return bool True if the name is a builtin, false otherwise.
 */
bool builtin_is_builtin(char *name);

/**
 * @brief Retrieves the function pointer for a given builtin command name.
 * @param name The command name string.
 * @return builtin_f_t The function pointer, or NULL if the command is not a builtin.
 */
builtin_f_t builtin_get_function(char *name);

/**
 * @brief Builtin command to change the current working directory.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_cd(int argc, char *argv[]);

/**
 * @brief Builtin command to display its arguments on the standard output.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status (always 0).
 */
int builtin_echo(int argc, char *argv[]);

/**
 * @brief Builtin command to terminate the shell process.
 * Sets the shell's exit flag and handles exit status argument.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_exit(int argc, char *argv[]);


/* ========================================================================== */
/*                              JOB CONTROL API                               */
/* ========================================================================== */

int builtin_fg(int argc, char *argv[]);

int builtin_bg(int argc, char *argv[]);

/**
 * @brief Builtin command to list active background jobs.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_jobs(int argc, char *argv[]);

/**
 * @brief Builtin command to display or manage the command history.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_history(int argc, char *argv[]);

/**
 * @brief Builtin command to print the name of the current working directory.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_pwd(int argc, char *argv[]);

/**
 * @brief Builtin command to describe how a command name will be interpreted.
 * Checks for builtins, external programs, etc.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int builtin_type(int argc, char *argv[]);

#endif // __BUILTIN_H__
