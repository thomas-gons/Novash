/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __HISTORY_H__
#define __HISTORY_H__

#define _GNU_SOURCE

#include "shell/config.h"
#include "shell/state.h"
#include "utils/system/memory.h"
#include <libgen.h>
#include <linux/limits.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/**
 * @brief Structure managing command line history.
 * * Uses a circular buffer approach for storing commands to enforce a maximum
 * size.
 */
typedef struct history_t {
  char **cmd_list;
  time_t *timestamps;
  size_t cmd_count;
  unsigned start;
  FILE *fp;
} history_t;

/**
 * @brief Initializes the history structure and loads commands from HISTFILE.
 * Allocates memory for the history buffer based on HISTORY_MAX_SIZE.
 */
void history_init();

/**
 * @brief Loads command history from the file specified by the HISTFILE
 * environment variable. Commands are read into the internal circular buffer.
 */
void history_load();

/**
 * @brief Adds a new command to the history buffer and immediately saves it to
 * the HISTFILE. Handles timestamping and internal buffer rotation (circular
 * buffer logic).
 * @param cmd The command string to save.
 */
void history_save_command(const char *cmd);

/**
 * @brief Trims the physical history file (HISTFILE) to ensure it doesn't exceed
 * the maximum allowed size (HISTORY_MAX_SIZE).
 */
void history_trim();

/**
 * @brief Frees all dynamically allocated resources used by the history module.
 * This includes the command strings, the timestamp array, and the history
 * structure itself.
 */
void history_free();

#endif // __HISTORY_H__