/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#include "history/history.h"
#include "builtin.h"

int builtin_history(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  history_t *hist = sh_state->hist;
  if (argc == 1) {
    for (unsigned i = 0; i < hist->cmd_count; i++) {
      unsigned idx = (hist->start + i) % HIST_SIZE;
      printf("%u  %s\n", i + 1, hist->cmd_list[idx]);
    }
  } else {
    if (strcmp(argv[1], "-c") == 0) {
      for (unsigned i = 0; i < hist->cmd_count; i++) {
        free(hist->cmd_list[i]);
        hist->cmd_list[i] = NULL;
      }

      hist->cmd_count = 0;
      hist->start = 0;
    }
  }
  return 0;
}