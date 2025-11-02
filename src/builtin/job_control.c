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


static bool parse_job_id(const char *arg, size_t *out_id, const char *cmd_name) {
  if (arg[0] != '%') {
    fprintf(stderr, "%s: invalid syntax, expected '%%<id>'\n", cmd_name);
    return false;
  }

  char *end;
  unsigned long val = strtoul(arg + 1, &end, 10);
  if (*end != '\0' || val == 0) {
    fprintf(stderr, "%s: invalid job id: %s\n", cmd_name, arg);
    return false;
  }

  *out_id = (size_t)val;
  return true;
}

static size_t *retrieve_arg_job_ids(const char *cmd_name, int argc, char *argv[]) {
  size_t jobs_count = (argc > 1) ? (size_t)(argc - 1) : 1;
  size_t *ids = xcalloc(jobs_count, sizeof(size_t));

  if (argc == 1) {
    ids[0] = 0; // "last stopped job"
    return ids;
  }

  for (int i = 1; i < argc; ++i) {
    if (!parse_job_id(argv[i], &ids[i - 1], cmd_name)) {
      free(ids);
      return NULL;
    }
  }
  return ids;
}

static inline job_t *find_target_job_fg(job_t *tail, size_t job_id) {
  for (job_t *job = tail; job; job = job->prev) {
    bool match = false;
    if (job_id == 0) {
      match = (job->state == JOB_STOPPED) ||
              (job->is_background && job->state == JOB_RUNNING);
    } else if (job->id == job_id) {
      match = (job->state == JOB_STOPPED) ||
              (job->is_background && job->state == JOB_RUNNING);
    }
    if (match)
      return job;
  }
  return NULL;
}

static inline job_t *find_target_job_bg(job_t *tail, size_t job_id) {
  for (job_t *job = tail; job; job = job->prev) {
    if ((job_id == 0 && job->state == JOB_STOPPED) ||
        (job_id != 0 && job->id == job_id && job->state == JOB_STOPPED))
      return job;
  }
  return NULL;
}

int builtin_bg(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();

  size_t *job_ids;
  if (argc > 1) {
    job_ids = retrieve_arg_job_ids("bg", argc, argv);
    if (!job_ids)
      return 1;
  } else {
    job_ids = xcalloc((size_t)1, sizeof(size_t));
    job_ids[0] = 0; // special value to indicate "last stopped job"
  }

  size_t jobs_to_process = (argc > 1) ? (size_t)(argc - 1) : 1;
  job_t *job;
  for (size_t i = 0; i < jobs_to_process; i++) {
    size_t job_id = job_ids[i];

    job = find_target_job_bg(sh_state->jobs_tail, job_id);

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
  }

  free(job_ids);
  return 0;
}

int builtin_fg(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  size_t *job_ids;
  if (argc > 1) {
    job_ids = retrieve_arg_job_ids("fg", argc, argv);
    if (!job_ids)
      return 1;
  } else {
    job_ids = xcalloc((size_t)1, sizeof(size_t));
    job_ids[0] = 0; // special value to indicate "last stopped job"
  }

  int status = 0;
  size_t jobs_to_process = (argc > 1) ? (size_t)(argc - 1) : 1;
  job_t *job;
  for (size_t i = 0; i < jobs_to_process; i++) {
    size_t job_id = job_ids[i];
    job = find_target_job_fg(sh_state->jobs_tail, job_id);
    if (!job) {
      fprintf(stderr, "fg: no stopped job\n");
      return 1;
    }

    xtcsetpgrp(STDIN_FILENO, job->pgid);

    // Continue the job if it was stopped
    if (!job->is_background) {
      xkill(-job->pgid, SIGCONT);
      jobs_mark_job_continued(job);
      jobs_print_job_status(job);
      job->state = JOB_RUNNING;
      sh_state->running_jobs_count++;
    } 
    // Job was running in background, just bring to foreground
    else {
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
    status = handle_foreground_execution(job, sfd);
  }

  free(job_ids);
  return status;
}
