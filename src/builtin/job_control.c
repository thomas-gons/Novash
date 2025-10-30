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
    for (unsigned i = 0; i < shell_state->jobs_count; i++) {
        printf("%s", jobs_job_str(job, i));
        job = job->next;
    }
    
    return 0;
}

int builtin_bg(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    job_t *job = shell_state->jobs;

    if (shell_state->jobs_count == 0) {
        printf("bg: no current job\n");
        return 1;
    }

    for (unsigned i = 0; i < shell_state->jobs_count; i++) {
        if (job->state == JOB_STOPPED) {
            kill(job->pgid, SIGCONT);
            job->state = JOB_RUNNING;
            shell_state->running_jobs_count++;
            printf("%s", jobs_job_str(job, i));
            break;
        }
        job = job->next;
    }
    return 0;
}

int builtin_fg(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    if (!shell_state->jobs_count) {
        printf("fg: no current job\n");
        return 1;
    }

    // Récupérer le dernier job
    job_t *job = shell_state->jobs;
    while (job->next) job = job->next;


    if (job->state == JOB_STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("kill(SIGCONT)");
            return 1;
        }
        job->state = JOB_RUNNING;
    }

    tcsetpgrp(STDIN_FILENO, job->pgid);

    int status;
    bool stopped = false;
    pid_t pid;

    // Attend tous les processus du groupe
    while ((pid = waitpid(-job->pgid, &status, WUNTRACED)) > 0) {

        process_t *p = job->first_process;
        while (p) {
            if (p->pid == pid) {
                if (WIFSTOPPED(status)) {
                    p->state = PROCESS_STOPPED;
                    stopped = true;
                } else if (WIFSIGNALED(status)) {
                    p->state = PROCESS_KILLED;
                } else if (WIFEXITED(status)) {
                    p->state = PROCESS_DONE;
                }
                p->status = status;
                break;
            }
            p = p->next;
        }

        if (stopped) break; // On arrête si un process a été stoppé
    }

    #include <errno.h>
    if (pid < 0 && errno != ECHILD) {
        perror("[fg] waitpid");
    }

    if (stopped) {
        job->state = JOB_STOPPED;
    } else {
        jobs_remove_job(job->pgid);
    }

    tcsetpgrp(STDIN_FILENO, getpid());

    return 0;
}