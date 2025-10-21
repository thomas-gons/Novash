#ifndef __SHELL_STATE_H__
#define __SHELL_STATE_H__

#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include "config.h"
#include "collections.h"


typedef enum {
    JOB_RUNNING,
    JOB_DONE,
    JOB_STOPPED,
    JOB_KILLED
} job_state_t;

typedef struct {
    pid_t pid;
    job_state_t state;
    char *cmd;
} job_t;

typedef struct {
    char *cmd_list[HIST_SIZE];
    time_t timestamps[HIST_SIZE];
    size_t cmd_count;
    unsigned start;
    FILE *fp;
} history_t;

typedef struct shell_state_t{
    hashmap_t *envp;

    char *current_dir;
    int last_exit_status;
    bool should_exit;

    history_t *hist;
    job_t *jobs;
    size_t jobs_count;
    size_t running_jobs_count;
} shell_state_t;

void shell_state_init();
shell_state_t *shell_state_get();
void shell_state_init_env();

#endif // __SHELL_STATE_H__
