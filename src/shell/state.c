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
      snprintf(hist_file_buf, PATH_MAX, "%s/%s", sh_state->identity.cwd, HIST_FILENAME);

  if (hist_file_len > 0 && hist_file_len < PATH_MAX) {
    shput(sh_state->environment, xstrdup("HISTFILE"), xstrdup(hist_file_buf));
  }
}

char *shell_state_getenv(const char *key) {
  return shget(sh_state->environment, key);
}

static void init_shell_identity() {
  shell_identity_t identity = {0};
  char cwd_buf[PATH_MAX];
  getcwd(cwd_buf, sizeof(cwd_buf));
	cwd_buf[sizeof(cwd_buf) - 1] = '\0';
	identity.cwd = xstrdup(cwd_buf);

  identity.hostname[0] = '\0';
  gethostname(identity.hostname, sizeof(identity.hostname));

  identity.username[0] = '\0';
  struct passwd *pw = getpwuid(getuid());
  if (pw) {
      strncpy(identity.username, pw->pw_name, sizeof(identity.username) - 1);
      identity.username[sizeof(identity.username) - 1] = '\0';
  }

  identity.uid = getuid();
  identity.gid = getgid();
  identity.pid = getpid();

  // Cannot call getpgrp() before setpgid(0,0) in shell_init()
  identity.pgid = -1;
  identity.argv0 = NULL;
  sh_state->identity = identity;
}

static void init_shell_jobs() {
	shell_jobs_t sh_jobs = {0};
	sh_jobs.jobs = NULL;
	sh_jobs.jobs_tail = NULL;
	sh_jobs.jobs_count = 0;
	sh_jobs.running_jobs_count = 0;
	sh_state->jobs = sh_jobs;
}

static void init_shell_last_exec() {
	shell_last_exec_t last_exec = {0};
	last_exec.exit_status = 0;
	last_exec.bg_pid = 0;
	last_exec.duration_ms = 0.0;
	last_exec.command = NULL;
	last_exec.started_at = (struct timespec){0};
	last_exec.ended_at = (struct timespec){0};
	sh_state->last_exec = last_exec;
}

void shell_state_init() {
  sh_state = xmalloc(sizeof(shell_state_t));
  sh_state->environment = NULL;
  sh_state->flags.interactive = isatty(STDIN_FILENO);
  sh_state->flags.job_control = sh_state->flags.interactive;
  sh_state->flags.history_enabled = true;
#if defined (LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_DEBUG
  sh_state->flags.debug = true;
#else
  sh_state->flags.debug = false;
#endif

  const char *locale = setlocale(LC_CTYPE, "");
  sh_state->support_utf8 = (locale != NULL && (strstr(locale, "UTF-8") || strstr(locale, "utf8")));
  sh_state->should_exit = false;
	
  init_shell_identity();
  init_environment();
  sh_state->identity.argv0 = shell_state_getenv("SHELL");
	
  sh_state->hist = xmalloc(sizeof(history_t));
  history_init();
  history_load();

  init_shell_jobs();
  init_shell_last_exec();
}

shell_identity_t *shell_state_get_identity() {
	return &sh_state->identity;
}

shell_jobs_t *shell_state_get_jobs() {
	return &sh_state->jobs;
}

shell_last_exec_t *shell_state_get_last_exec() {
	return &sh_state->last_exec;
}

void shell_reset_last_exec() {
  shell_last_exec_t *last_exec = &sh_state->last_exec;
  if (last_exec->command) {
    free(last_exec->command);
    last_exec->command = NULL;
  }
  
  last_exec->exit_status = 0;
  last_exec->bg_pid = 0;
  last_exec->duration_ms = 0.0;
  last_exec->started_at = (struct timespec){0};
  last_exec->ended_at = (struct timespec){0};
}

char *shell_state_get_flags(void) {
    shell_state_t *sh = shell_state_get();
    char buf[8];  // assez pour 4-5 flags + '\0'
    size_t pos = 0;

    if (sh->flags.interactive)     buf[pos++] = 'i';
    if (sh->flags.job_control)     buf[pos++] = 'm';
    if (sh->flags.history_enabled) buf[pos++] = 'h';
    if (sh->flags.debug)           buf[pos++] = 'd';

    buf[pos] = '\0';
    return xstrdup(buf);
}

bool shell_is_utf8_supported() {
  return sh_state->support_utf8;
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

  free(sh_state->identity.cwd);
  shell_reset_last_exec();
  history_free();
  jobs_free();

  free(sh_state);
}