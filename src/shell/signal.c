/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "signal.h"

void handle_sigint_event() {
  // Ensure the terminal is clean after interrupt in the readline context
  rl_on_new_line();
  rl_replace_line("", 0);
  rl_redisplay(); // Force prompt redisplay
}

void handle_sigchld_events() {
  int status;
  pid_t pid;
  /* Reap any child (or any process in any group) that changed state */
  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
    process_t *p = jobs_find_process_by_pid(pid);
    job_t *job = p->parent_job;

    if (p == NULL) {
      fprintf(stderr, "reaper: unknown pid %d\n", (int)pid);
      continue;
    }

    if (WIFEXITED(status)) {
      p->state = PROCESS_DONE;
      p->status = WEXITSTATUS(status);
      pr_info("reaper: pid %d exited with status %d", (int)pid, p->status);
      job->live_processes--;
    } else if (WIFSIGNALED(status)) {
      p->state = PROCESS_KILLED;
      p->status = WTERMSIG(status);
      pr_warn("reaper: pid %d killed by signal %d", (int)pid, p->status);
      job->live_processes--;
    } else if (WIFSTOPPED(status)) {
      p->state = PROCESS_STOPPED;
      pr_info("reaper: pid %d stopped by signal %d", (int)pid,
              WSTOPSIG(status));
      jobs_mark_job_stopped(job);
      job->live_processes = 0; // All processes considered stopped
    } else if (WIFCONTINUED(status)) {
      p->state = PROCESS_RUNNING;
      pr_info("reaper: pid %d continued", (int)pid);
    } else {
      pr_warn("reaper: pid %d unknown status 0x%x", (int)pid, status);
    }

    if (job->live_processes == 0 && job->is_background) {
      jobs_mark_job_completed(job);
      rl_forced_update_display();
    }
  }

  if (pid < 0 && errno != ECHILD) {
    perror("waitpid");
  }
}
