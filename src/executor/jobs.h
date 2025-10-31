/*
 * Novash — Minimalist shell
 * Copyright (C) 2025 Thomas Gons
 * Licensed under the GPLv3 or later.
 *
 * Job control: structures and APIs to manage jobs, processes, and pipelines.
 * Handles foreground/background execution and process states.
 */

#ifndef NOVASH_JOBS_H
#define NOVASH_JOBS_H

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <sys/types.h>
#include "utils/collections.h"
#include "parser/parser.h"
#include "shell/state.h"

// Process states
typedef enum {
    PROCESS_RUNNING,
    PROCESS_DONE,
    PROCESS_STOPPED,
    PROCESS_KILLED
} process_state_e;

// Job states
typedef enum {
    JOB_RUNNING,
    JOB_DONE,
    JOB_STOPPED,
    JOB_KILLED
} job_state_e;

// Single process in a job
typedef struct process_t {
    pid_t pid;                 // Process ID
    char **argv;               // Command arguments
    redirection_t *redir;      // I/O redirections
    process_state_e state;     // Process state
    int status;                // Exit status or signal
    struct process_t *next;    // Next process in pipeline
    struct job_t *parent_job;  // Parent job
} process_t;

// Job: pipeline of processes
typedef struct job_t {
    unsigned id;               // Unique job ID
    pid_t pgid;                // Process group ID
    process_t *first_process;  // First process in pipeline
    char *command;             // Original command line
    bool is_background;        // True if background job
    job_state_e state;         // Job state
    unsigned live_processes;   // Count of running processes
    struct job_t *prev;        // Previous job in job list
    struct job_t *next;        // Next job in job list
} job_t;

// Process APIs
process_t *jobs_new_process(cmd_node_t *cmd, bool deep_copy);
void jobs_add_process_to_job(job_t *job, process_t *process);
void jobs_free_process(process_t *process, bool deep_free);

// Job APIs
job_t *jobs_new_job();
void jobs_add_job(job_t *job);
bool jobs_remove_job(pid_t pgid);
job_t *jobs_last_job();
job_t *jobs_second_last_job();
job_t *jobs_find_job_by_pgid(pid_t pgid);
void jobs_free_job(job_t *job, bool deep_free);
char *jobs_job_str(job_t *job);
void jobs_free();

// Job state helpers
void jobs_mark_job_stopped(job_t *job);
void jobs_mark_job_completed(job_t *job);
process_t *jobs_find_process_by_pid(pid_t pid);
job_t *jobs_find_job_by_pgid(pid_t pgid);

#endif /* NOVASH_JOBS_H */
