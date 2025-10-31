/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "jobs.h"

static unsigned next_job_id = 1;

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
  job->id = next_job_id++;

  // other fields are zeroed by xcalloc
  return job;
}

// --- Job list management with doubly-linked list ---
void jobs_add_job(job_t *job) {
  shell_state_t *sh_state = shell_state_get();
  job->next = NULL;
  job->prev = sh_state->jobs_tail;

  if (!sh_state->jobs) {
    sh_state->jobs = job;
  } else {
    sh_state->jobs_tail->next = job;
  }
  sh_state->jobs_tail = job;
  sh_state->jobs_count++;
}

bool jobs_remove_job(pid_t pgid) {
  shell_state_t *sh_state = shell_state_get();
  job_t *job = jobs_find_job_by_pgid(pgid);
  if (!job)
    return false;

  if (job->prev)
    job->prev->next = job->next;
  else
    sh_state->jobs = job->next;

  if (job->next)
    job->next->prev = job->prev;
  else
    sh_state->jobs_tail = job->prev;

  jobs_free_job(job, true);
  sh_state->jobs_count--;
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
  shell_state_t *sh_state = shell_state_get();
  job_t *job = sh_state->jobs;
  while (job) {
    job_t *next = job->next;
    jobs_free_job(job, true);
    job = next;
  }
  sh_state->jobs = NULL;
  sh_state->jobs_tail = NULL;
  sh_state->jobs_count = 0;
  sh_state->running_jobs_count = 0;
}

char *jobs_job_str(job_t *job) {
  shell_state_t *sh_state = shell_state_get();

  char active = ' ';
  if (job == sh_state->jobs_tail)
    active = '+';
  else if (job == sh_state->jobs_tail->prev)
    active = '-';

  unsigned job_no = 1;
  for (job_t *j = sh_state->jobs; j; j = j->next) {
    if (j == job)
      break;
    job_no++;
  }

  char *state_str;
  switch (job->state) {
  case JOB_RUNNING:
    state_str = "running";
    break;
  case JOB_STOPPED:
    state_str = "stopped";
    break;
  case JOB_DONE:
    state_str = "  done ";
    break;
  case JOB_KILLED:
    state_str = " killed";
    break;
  default:
    return strdup("[Error: Unknown job state]\n");
  }

  char *buf;
  if (asprintf(&buf, "[%d] %c %7s %s\n", job_no, active, state_str,
               job->command) < 0) {
    return strdup("[Error: asprintf failed]\n");
  }
  return buf;
}

job_t *jobs_last_job() { return shell_state_get()->jobs_tail; }

job_t *jobs_second_last_job() {
  job_t *tail = shell_state_get()->jobs_tail;
  return tail ? tail->prev : NULL;
}

job_t *jobs_find_job_by_pgid(pid_t pgid) {
  for (job_t *job = shell_state_get()->jobs_tail; job; job = job->prev) {
    if (job->pgid == pgid)
      return job;
  }
  return NULL;
}

process_t *jobs_find_process_by_pid(pid_t pid) {
  for (job_t *job = shell_state_get()->jobs_tail; job; job = job->prev) {
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

  char *buf = jobs_job_str(job);
  printf("%s", buf);
  free(buf);

  shell_state_get()->running_jobs_count--;
}

void jobs_mark_job_completed(job_t *job) {
  job->state = JOB_DONE;

  char *buf = jobs_job_str(job);
  printf("%s", buf);
  free(buf);

  jobs_remove_job(job->pgid);
}
