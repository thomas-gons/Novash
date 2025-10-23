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
    char *buf;
    shell_state_t *shell_state = shell_state_get();
    job_t *jobs = shell_state->jobs;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        unsigned job_id = 0;
        for (unsigned i = 0; i < shell_state->jobs_count; i++) {
            if (jobs[i].pid == pid) { job_id = i; break; }
        }

        if (job_id == shell_state->jobs_count) {
            job_t stopped_job = (job_t){ .cmd = "", .pid = pid, .state = 0 };
            shell_state->jobs[job_id] = stopped_job;
            buf = job_str(stopped_job, job_id);
            shell_state->jobs_count++;
            continue;
        }

        job_t *job = &jobs[job_id];
        if (WIFSTOPPED(status)) job->state = JOB_STOPPED;
        else if (WIFSIGNALED(status)) job->state = JOB_KILLED;
        else job->state = JOB_DONE;
        shell_state->running_jobs_count--;

        buf = job_str(*job, job_id);
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, "\n", 1);

        // --- Redraw the prompt and preserve current input ---
        rl_on_new_line();                   // move to new line
        rl_replace_line(rl_line_buffer, 0); // keep current line
        rl_redisplay();                     // redraw prompt + input
    }
}

void sigint_handler(int sig) {
    (void)sig;
    shell_state_t *shell_state = shell_state_get();
    shell_state->sigint_received = true; 
}

void sigchld_handler(int sig) {
    (void)sig;
    shell_state_t *shell_state = shell_state_get();
    shell_state->sigchld_received = true;
    if (rl_line_buffer != NULL) {
        rl_done = 1;
    }
}
