/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __JOBS_H__
#define __JOBS_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include "shell/state.h"


typedef enum {
    JOB_RUNNING,
    JOB_DONE,
    JOB_STOPPED,
    JOB_KILLED
} job_state_t;

typedef struct job_t {
    pid_t pid;
    job_state_t state;
    char *cmd;
} job_t;

/**
 * @brief Formats a job_t structure into a string for display in the shell.
 * * @param job The job structure to format.
 * @param job_id The 0-based index of the job in the jobs list.
 * @return char* A heap-allocated string containing the formatted job status.
 */
char *job_str(job_t job, unsigned job_id);


#endif // __JOBS_H__