/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "jobs.h"

process_t *jobs_new_process(cmd_node_t *cmd, bool deep_copy) {
  process_t *process = xcalloc(1, sizeof(process_t));

  char **argv_cp = NULL;
  if (deep_copy) {
    argv_cp = NULL;
    for (int i = 0; i < arrlen(cmd->argv); i++) {
      arrpush(argv_cp, xstrdup(cmd->argv[i]));
    }
    arrpushnc(argv_cp, NULL);
  } else {
    argv_cp = cmd->argv;
    arrpushnc(argv_cp, NULL);
  }

  redirection_t *redir_cp = NULL;
  if (deep_copy) {
    int redir_count = (int)arrlen(cmd->redir);
    if (redir_count > 0) {
      redir_cp = NULL;
      for (int i = 0; i < redir_count; i++) {
        redirection_t r = {.fd = cmd->redir[i].fd,
                           .type = cmd->redir[i].type,
                           .target = xstrdup(cmd->redir[i].target)};
        arrpush(redir_cp, r);
      }
    }
  } else {
    redir_cp = cmd->redir;
  }

  process->argv = argv_cp;
  process->redir = redir_cp;

  // xcalloc already set other fields to 0/NULL
  return process;
}

void jobs_add_process_to_job(job_t *job, process_t *process) {
  if (!job->first_process) {
    job->first_process = process;
  } else {
    process_t *ptr = job->first_process;
    while (ptr->next)
      ptr = ptr->next;
    ptr->next = process;
  }
}

void jobs_free_process(process_t *process, bool deep_free) {
  if (!process)
    return;

  if (deep_free) {
    if (process->argv) {
      for (size_t i = 0; process->argv[i] != NULL; i++) {
        free(process->argv[i]);
      }
      arrfree(process->argv);
    }
    if (process->redir) {
      for (size_t i = 0; process->redir[i].target != NULL; i++) {
        free(process->redir[i].target);
      }
      arrfree(process->redir);
    }
  }
  free(process);
}

job_t *jobs_new_job() {
  job_t *job = xcalloc(1, sizeof(job_t));
  if (!job)
    return NULL;

  job->state = JOB_RUNNING;
  // other fields are zeroed by xcalloc
  return job;
}

// --- Job list management with doubly-linked list ---
void jobs_add_job(job_t *job) {
  shell_jobs_t *sh_jobs = shell_state_get_jobs();
  job->next = NULL;
  job->prev = sh_jobs->jobs_tail;

  size_t new_id = 1;
  for (job_t *j = sh_jobs->jobs; j != NULL; j = j->next) {
    if (j->id >= new_id + 1) {
      break;
    }
    new_id++;
  }
  job->id = new_id;

  if (!sh_jobs->jobs) {
    sh_jobs->jobs = job;
  } else {
    sh_jobs->jobs_tail->next = job;
  }
  sh_jobs->jobs_tail = job;
  sh_jobs->jobs_count++;
  sh_jobs->running_jobs_count++;
}

bool jobs_remove_job(pid_t pgid) {
  shell_jobs_t *sh_jobs = shell_state_get_jobs();
  job_t *job = jobs_find_job_by_pgid(pgid);
  if (!job)
    return false;

  if (job->prev)
    job->prev->next = job->next;
  else
    sh_jobs->jobs = job->next;

  if (job->next)
    job->next->prev = job->prev;
  else
    sh_jobs->jobs_tail = job->prev;

  sh_jobs->jobs_count--;
  if (job->state != JOB_STOPPED) {
    sh_jobs->running_jobs_count--;
  }
  jobs_free_job(job, true);
  return true;
}

void jobs_free_job(job_t *job, bool deep_free) {
  if (!job)
    return;

  process_t *p = job->first_process;
  while (p) {
    process_t *next = p->next;
    jobs_free_process(p, deep_free);
    p = next;
  }

  if (deep_free && job->command) {
    free(job->command);
  }

  free(job);
}

void jobs_free() {
  shell_jobs_t *sh_jobs = shell_state_get_jobs();
  job_t *job = sh_jobs->jobs;

  while (job) {
    job_t *next = job->next;
    jobs_free_job(job, true);
    job = next;
  }
  sh_jobs->jobs = NULL;
  sh_jobs->jobs_tail = NULL;
  sh_jobs->jobs_count = 0;
  sh_jobs->running_jobs_count = 0;
}

void jobs_print_job_status(job_t *job) {
  shell_jobs_t *sh_jobs = shell_state_get_jobs();

  char active = ' ';
  if (job == sh_jobs->jobs_tail)
    active = '+';
  else if (job == sh_jobs->jobs_tail->prev)
    active = '-';

  char *state_str;
  switch (job->state) {
  case JOB_RUNNING:
    state_str = " running ";
    break;
  case JOB_DONE:
    state_str = "   done  ";
    break;
  case JOB_STOPPED:
    state_str = " stopped ";
    break;
  case JOB_CONTINUED:
    state_str = "continued";
    break;
  case JOB_KILLED:
    state_str = "  killed ";
    break;
  default:
    printf("Unknown job state: %d\n", job->state);
    return;
  }

  printf("[%zu] %c %9s %s\n", job->id, active, state_str, job->command);
}

job_t *jobs_last_job() { return shell_state_get_jobs()->jobs_tail; }

job_t *jobs_second_last_job() {
  job_t *tail = shell_state_get_jobs()->jobs_tail;
  return tail ? tail->prev : NULL;
}

job_t *jobs_find_job_by_pgid(pid_t pgid) {
  for (job_t *job = shell_state_get_jobs()->jobs_tail; job; job = job->prev) {
    if (job->pgid == pgid)
      return job;
  }
  return NULL;
}

process_t *jobs_find_process_by_pid(pid_t pid) {
  for (job_t *job = shell_state_get_jobs()->jobs_tail; job; job = job->prev) {
    for (process_t *p = job->first_process; p; p = p->next) {
      if (p->pid == pid)
        return p;
    }
  }
  return NULL;
}

void jobs_mark_job_stopped(job_t *job) {
  for (process_t *p = job->first_process; p; p = p->next) {
    if (p->state == PROCESS_RUNNING)
      p->state = PROCESS_STOPPED;
  }
  job->state = JOB_STOPPED;

  jobs_print_job_status(job);

  shell_state_get_jobs()->running_jobs_count--;
}

void jobs_mark_job_continued(job_t *job) {
  for (process_t *p = job->first_process; p; p = p->next) {
    if (p->state == PROCESS_STOPPED) {
      p->state = PROCESS_RUNNING;
      job->live_processes++;
    }
  }
  job->state = JOB_CONTINUED;
}

void jobs_mark_job_completed(job_t *job) {
  job->state = JOB_DONE;

  jobs_print_job_status(job);
  jobs_remove_job(job->pgid);
}
