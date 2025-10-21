#include "shell_state.h"

static shell_state_t *ss;

void shell_state_init() {
    ss = xmalloc(sizeof(shell_state_t));
    ss->envp = hashmap_new(256);
    ss->current_dir = NULL;
    ss->last_exit_status = 0;
    ss->should_exit = false;
    
    ss->hist = xmalloc(sizeof(history_t));
    ss->jobs_count = 0;
    ss->jobs = xmalloc(sizeof(job_t) * MAX_JOBS);
    ss->running_jobs_count = 0;
}

shell_state_t *shell_state_get() {
    return ss;
}

void shell_state_init_env() {
    hashmap_set(ss->envp, "HOME", getenv("HOME"));
    hashmap_set(ss->envp, "PATH", getenv("PATH"));
}