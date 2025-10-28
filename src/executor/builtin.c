/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "builtin.h"


static builtin_t builtins[] = {
    {"cd", builtin_cd},
    {"echo", builtin_echo},
    {"exit", builtin_exit},
    {"bg", builtin_bg},
    {"fg", builtin_fg},
    {"jobs", builtin_jobs},
    {"history", builtin_history},
    {"pwd", builtin_pwd},
    {"type", builtin_type},
};

#define BUILTINS_N (int) (sizeof(builtins) / sizeof(builtin_t))


int builtin_cd(int argc, char *argv[]) {
    const char *home = shell_state_getenv("HOME");
    char *_path = argv[1];
    int r = chdir(argc == 1 || *_path == '~' ? home: _path);
    return r;
}

int builtin_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++)
        printf("%s ", argv[i]);
    
    printf("\n");
    return 0;
}

int builtin_exit(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    shell_state->should_exit = true;
    return 0;
}

/* --- JOB_CONTROL --- */

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
                    p->stopped = true;
                    stopped = true;
                } else if (WIFSIGNALED(status)) {
                    p->completed = true;
                } else if (WIFEXITED(status)) {
                    p->completed = true;
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


int builtin_jobs(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    job_t *job = shell_state->jobs;
    for (unsigned i = 0; i < shell_state->jobs_count; i++) {
        printf("%s", jobs_job_str(job, i));
        job = job->next;
    }
    
    return 0;
}

int builtin_history(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    history_t *hist = shell_state->hist;
    if (argc == 1) {
        for (unsigned i = 0; i < hist->cmd_count; i++) {
            unsigned idx = (hist->start + i) % HIST_SIZE;
            printf("%u  %s\n", i+1, hist->cmd_list[idx]);
        }
    } else {
        if (strcmp(argv[1], "-c") == 0) {
            for (unsigned i = 0; i < hist->cmd_count; i++) {
                free(hist->cmd_list[i]);
                hist->cmd_list[i] = NULL;
            }

            hist->cmd_count = 0;
            hist->start = 0;
        }

    }
    return 0;
}

int builtin_pwd(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    printf("%s\n", shell_state->cwd);
    return 0;
}

int builtin_type(int argc, char *argv[]) {
    if (argc < 2) {
        printf("type: missing argument\n");
        return 1;
    }

    char *cmd = argv[1];

    if (builtin_is_builtin(cmd)) {
        printf("%s is a shell builtin\n", cmd);
    } else {
        char *cmd_path = is_in_path(cmd);
        if (cmd_path) {
            printf("%s is %s\n", cmd, cmd_path);
            free(cmd_path);
        } else {
            printf("%s: not found\n", cmd);
        }
    }
    return 0;
}


bool builtin_is_builtin(char *name) {
    for (int i = 0; i < BUILTINS_N; i++) {
        if (strcmp(name, builtins[i].name) == 0)
            return true;
    }
    return false;
}

builtin_f_t builtin_get_function(char *name) {
    for (int i = 0; i < BUILTINS_N; i++) {
        if (strcmp(name, builtins[i].name) == 0)
            return builtins[i].f;
    }
    return NULL;
}