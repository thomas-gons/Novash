/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include "shell/state.h"
#include "utils/memory.h"


/**
 * @brief Searches for an executable command in the directories listed in the PATH
 * environment variable.
 * @param cmd The command name (e.g., "ls").
 * @return The heap-allocated absolute path of the command if found and executable, 
 * or NULL otherwise. The caller is responsible for freeing the returned string.
 */
char *is_in_path(char *cmd);

#endif // __UTILS_H__