/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#ifndef __PS1_H__
#define __PS1_H__

#include "utils/system/memory.h"
#include <locale.h>
#include <shell/state.h>
#include <stdbool.h>

#define USE_UTF8_SYMBOLS true
#define USE_COLORS true

#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

#define BG_COLOR_BLACK "\033[40m"
#define BG_COLOR_RED "\033[41m"
#define BG_COLOR_GREEN "\033[42m"
#define BG_COLOR_YELLOW "\033[43m"
#define BG_COLOR_BLUE "\033[44m"
#define BG_COLOR_MAGENTA "\033[45m"
#define BG_COLOR_CYAN "\033[46m"
#define BG_COLOR_WHITE "\033[47m"

#define COLOR_RESET "\033[0m"

typedef struct {
  const char *fg;
  const char *bg;
} PS1_color_t;

typedef struct {
  char *text;
  const char *leading;
  const char *trailing;
  PS1_color_t color;
} PS1_block_t;

void prompt_symbols_init();
char *prompt_build_ps1();

#endif // __PS1_H__