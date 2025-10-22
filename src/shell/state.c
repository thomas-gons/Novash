/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#define _DEFAULT_SOURCE

#include "shell/state.h"
#include "history/history.h"
#include "executor/jobs.h"

static shell_state_t *ss;

shell_state_t *shell_state_get() {
    if (ss == NULL) {
        fprintf(stderr, "Fatal Error: Shell instance was not initialized before use.\n");    
        exit(EXIT_FAILURE);
    }
    return ss;
}

/**
 * @brief Initializes the shell's environment hashmap with essential variables.
 * * HOME - absolute path to the current user's home.
 * * PATH - the colon-separated list of directories for executable searches
 * * SHELL - the absolute path to the Novash executable itself. 
 * * HISTFILE - the absolute path to the file used for storing command history.
 * All keys and values are duplicated (xstrdup) before being stored in the hashmap.
 */
static void init_environment() {
    // --- HOME and PATH ---
    char *home_val = getenv("HOME");
    if (home_val) {
        hashmap_set(ss->environment, xstrdup("HOME"), xstrdup(home_val));
    }
    char *path_val = getenv("PATH");
    if (path_val) {
        hashmap_set(ss->environment, xstrdup("PATH"), xstrdup(path_val));
    }

    // --- SHELL ---
    char exe_buf[PATH_MAX];
    ssize_t exe_len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);

    if (exe_len != -1) {
        exe_buf[exe_len] = '\0'; 
        hashmap_set(ss->environment, xstrdup("SHELL"), xstrdup(exe_buf));
    }

    // --- HISTFILE ---
    char hist_file_buf[PATH_MAX];
    int hist_file_len = snprintf(
        hist_file_buf, 
        PATH_MAX, 
        "%s/%s", 
        ss->cwd,
        HIST_FILENAME
    );
    
    if (hist_file_len > 0 && hist_file_len < PATH_MAX) {
        hashmap_set(ss->environment, xstrdup("HISTFILE"), xstrdup(hist_file_buf)); // FIX C
    }
}

char *shell_state_getenv(const char *key) {
    return hashmap_get(ss->environment, key);
}

void shell_state_init() {
    ss = xmalloc(sizeof(shell_state_t));
    ss->environment = hashmap_new(256);
    
    char cwd_buf[PATH_MAX];
    getcwd(cwd_buf, sizeof(cwd_buf));
    ss->cwd = xstrdup(cwd_buf);
    
    ss->last_exit_status = 0;
    ss->should_exit = false;
    
    ss->hist = xmalloc(sizeof(history_t));    
    ss->jobs_count = 0;
    ss->jobs = xmalloc(sizeof(job_t) * MAX_JOBS);
    ss->running_jobs_count = 0;

    init_environment();
    history_init();
    history_load();
}

void shell_state_free() {
    hashmap_free(ss->environment);

    free(ss->cwd);
    history_free();
    free(ss->jobs);

    free(ss);
}