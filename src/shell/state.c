/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#define _DEFAULT_SOURCE

#include "shell/state.h"
#include "executor/jobs.h"
#include "history/history.h"

static shell_state_t *sh_state = NULL;

shell_state_t *shell_state_get() {
  if (sh_state == NULL) {
    fprintf(stderr,
            "Fatal Error: Shell instance was not initialized before use.\n");
    exit(EXIT_FAILURE);
  }
  return sh_state;
}

/**
 * @brief Initializes the shell's environment hashmap with essential variables.
 * * HOME - absolute path to the current user's home.
 * * PATH - the colon-separated list of directories for executable searches
 * * SHELL - the absolute path to the Novash executable itself.
 * * HISTFILE - the absolute path to the file used for storing command history.
 * All keys and values are duplicated (xstrdup) before being stored in the
 * hashmap.
 */
static void init_environment() {
  // --- HOME and PATH ---
  char *home_val = getenv("HOME");
  if (home_val) {
    shput(sh_state->environment, xstrdup("HOME"), xstrdup(home_val));
  }
  char *path_val = getenv("PATH");
  if (path_val) {
    shput(sh_state->environment, xstrdup("PATH"), xstrdup(path_val));
  }

  // --- SHELL ---
  char exe_buf[PATH_MAX];
  ssize_t exe_len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);

  if (exe_len != -1) {
    exe_buf[exe_len] = '\0';
    shput(sh_state->environment, xstrdup("SHELL"), xstrdup(exe_buf));
  }

  // --- HISTFILE ---
  char hist_file_buf[PATH_MAX];
  int hist_file_len =
      snprintf(hist_file_buf, PATH_MAX, "%s/%s", sh_state->cwd, HIST_FILENAME);

  if (hist_file_len > 0 && hist_file_len < PATH_MAX) {
    shput(sh_state->environment, xstrdup("HISTFILE"), xstrdup(hist_file_buf));
  }
}

char *shell_state_getenv(const char *key) {
  return shget(sh_state->environment, key);
}

void shell_state_init() {
  sh_state = xmalloc(sizeof(shell_state_t));
  sh_state->environment = NULL;

  char cwd_buf[PATH_MAX];
  getcwd(cwd_buf, sizeof(cwd_buf));
  sh_state->cwd = xstrdup(cwd_buf);
  sh_state->last_fg_cmd = NULL;
  sh_state->last_exit_status = 0;
  sh_state->should_exit = false;

  sh_state->hist = xmalloc(sizeof(history_t));
  sh_state->jobs_count = 0;
  sh_state->jobs = NULL;
  sh_state->jobs_tail = NULL;
  sh_state->running_jobs_count = 0;

  init_environment();
  history_init();
  history_load();
}

void shell_regain_control() {
  // Regain control of the terminal
  xtcsetpgrp(STDIN_FILENO, getpgrp());

  xtcsetattr(STDIN_FILENO, TCSADRAIN, &sh_state->shell_tmodes);
}

void shell_state_free() {
  // Free environment hashmap
  for (int i = 0; i < shlen(sh_state->environment); i++) {
    env_var_t env_var = sh_state->environment[i];
    free(env_var.key);
    free(env_var.value);
  }
  shfree(sh_state->environment);

  free(sh_state->cwd);
  history_free();
  jobs_free();

  free(sh_state);
}