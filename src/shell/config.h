/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define HIST_SIZE 1000
#define COMMAND_NOT_FOUND_EXIT_CODE 127
#define JOB_STOPPED_EXIT_CODE 146

#define TRAILING_DIAMOND "\uE0B4"
#define LEADING_DIAMOND  "\uE0B6"
#define TRAILING_POWERLINE "\uE0B0"
#define LEADING_POWERLINE  "\uE0B1"

// define ps1 format
#define PS1 \
    "\\BG_G "                  \
    "\\u@\\h "                 \
    "\\BG_X\\FG_G\\BG_B"       \
    TRAILING_DIAMOND           \
    "\\FG_X"                   \
    " "                        \
    "\\W "                     \
    "\\BG_X\\FG_B"             \
    TRAILING_DIAMOND          \
    "\\FG_X"                   \
    " "
#define HIST_FILENAME ".nsh_history"

#endif // __CONFIG_H__