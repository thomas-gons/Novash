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

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

        job_t *job = shell_state->jobs;
        unsigned job_id;
        for (job_id = 0; job != NULL; job_id++, job = job->next) {
            if (job->pgid == pid) break;
        }
        if (job == NULL) continue;

        if (WIFSTOPPED(status)) job->state = JOB_STOPPED;
        else if (WIFSIGNALED(status)) job->state = JOB_KILLED;
        else job->state = JOB_DONE;
        shell_state->running_jobs_count--;

        buf = jobs_job_str(job, job_id);
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, "\n", 1);
        free(buf);
        
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
