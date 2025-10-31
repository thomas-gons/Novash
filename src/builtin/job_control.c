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
#include "executor/executor.h"
#include "executor/jobs.h"

int builtin_jobs(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  job_t *job = sh_state->jobs;
  while (job) {
    jobs_print_job_status(job);
    job = job->next;
  }

  return 0;
}

int builtin_bg(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  job_t *job = sh_state->jobs_tail;
  while (job && job->state != JOB_STOPPED) {
    job = job->prev;
  }

  if (!job) {
    fprintf(stderr, "bg: no stopped job\n");
    return 1;
  }
  xkill(-job->pgid, SIGCONT);
  jobs_mark_job_continued(job);
  job->is_background = true;
  jobs_print_job_status(job);
  job->state = JOB_RUNNING;
  sh_state->running_jobs_count++;
  return 0;
}

int builtin_fg(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  job_t *job = sh_state->jobs_tail;
  bool from_bg = false;
  while (job && job->state != JOB_STOPPED) {
    if (job->is_background && job->state == JOB_RUNNING) {
      from_bg = true;
      break;
    }
    job = job->prev;
  }

  if (!job) {
    fprintf(stderr, "fg: no stopped job\n");
    return 1;
  }

  xtcsetpgrp(STDIN_FILENO, job->pgid);
  if (!from_bg) {
    xkill(-job->pgid, SIGCONT);
    jobs_mark_job_continued(job);
    jobs_print_job_status(job);
    job->state = JOB_RUNNING;
    sh_state->running_jobs_count++;
  } else {
    job->is_background = false;
    jobs_print_job_status(job);
  }

  sigset_t mask, prev_mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTSTP);
  xsigprocmask(SIG_BLOCK, &mask, &prev_mask);

  int sfd = xsignalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  return handle_foreground_execution(job, sfd);
}