/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/wait.h>

#include "builtin.h"
#include "executor/jobs.h"

int builtin_jobs(int argc, char *argv[]) {
  shell_state_t *shell_state = shell_state_get();
  job_t *job = shell_state->jobs;
  while (job) {
    char *buf = jobs_job_str(job);
    printf("%s", buf);
    free(buf);
    job = job->next;
  }

  return 0;
}

int builtin_bg(int argc, char *argv[]) {
  // TODO: implement bg builtin
  return 0;
}

int builtin_fg(int argc, char *argv[]) {
  // TODO: implement fg builtin
  return 0;
}