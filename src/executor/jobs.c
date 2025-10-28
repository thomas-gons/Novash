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
        size_t argc = arr_len(cmd->argv);
        argv_cp = xmalloc((argc + 1) * sizeof(char *));
        for (size_t i = 0; i < argc; i++) {
            arr_push(argv_cp, xstrdup(cmd->argv[i]));
        }
        argv_cp[argc] = NULL; // ensure NULL-terminated
    } else {
        argv_cp = cmd->argv;
    }

    redirection_t *redir_cp = NULL;
    if (deep_copy) {
        size_t redir_count = arr_len(cmd->redir);
        if (redir_count > 0) {
            redir_cp = xmalloc((redir_count + 1) * sizeof(redirection_t)); // +1 for sentinel
            for (size_t i = 0; i < redir_count; i++) {
                redirection_t r = {
                    .fd = cmd->redir[i].fd,
                    .type = cmd->redir[i].type,
                    .target = xstrdup(cmd->redir[i].target)
                };
                arr_push(redir_cp, r);
            }
            redir_cp[redir_count] = (redirection_t){ .fd = -1, .target = NULL }; // sentinel
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
        while (ptr->next) ptr = ptr->next;
        ptr->next = process;
    }
}

void jobs_free_process(process_t *process, bool deep_free) {
    if (!process) return;

    if (deep_free) {
        if (process->argv) {
            for (size_t i = 0; process->argv[i] != NULL; i++) {
                free(process->argv[i]);
            }
            free(process->argv);
        }
        if (process->redir) {
            for (size_t i = 0; process->redir[i].target != NULL; i++) {
                free(process->redir[i].target);
            }
            free(process->redir);
        }
    }
    free(process);
}

job_t *jobs_new_job() {
    job_t *job = xcalloc(1, sizeof(job_t));
    if (!job) return NULL;

    job->state = JOB_RUNNING;

    // other fields are zeroed by xcalloc
    return job;
}

void jobs_add_job(job_t *job) {
    shell_state_t *shell_state = shell_state_get();
    job->next = NULL; // security
    if (shell_state->jobs_count == 0) {
        shell_state->jobs = job;
    } else {
        job_t *ptr = shell_state->jobs;
        while (ptr->next) ptr = ptr->next;
        ptr->next = job;
    }
    shell_state->jobs_count++;
}

bool jobs_remove_job(pid_t pgid) {
    shell_state_t *shell_state = shell_state_get();
    if (shell_state->jobs_count == 0) return false;

    job_t *job_ptr = shell_state->jobs;
    job_t *prev_job_ptr = NULL;

    while (job_ptr != NULL) {
        if (job_ptr->pgid == pgid) {
            if (!prev_job_ptr) {
                shell_state->jobs = job_ptr->next;
            } else {
                prev_job_ptr->next = job_ptr->next;
            }
            free(job_ptr);
            shell_state->jobs_count--;
            return true;
        }
        prev_job_ptr = job_ptr;
        job_ptr = job_ptr->next;
    }
    return false;
}

job_t *jobs_get_job(pid_t pgid) {
    shell_state_t *shell_state = shell_state_get();
    if (shell_state->jobs_count == 0) return NULL;

    job_t *job_ptr = shell_state->jobs;
    while (job_ptr != NULL) {
        if (job_ptr->pgid == pgid) {
            return job_ptr;
        }
        job_ptr = job_ptr->next;
    }
    return NULL;
}

void jobs_free_job(job_t *job, bool deep_free) {
    if (!job) return;

    process_t *proc_ptr = job->first_process;
    while (proc_ptr != NULL) {
        process_t *next_proc = proc_ptr->next;
        jobs_free_process(proc_ptr, deep_free);
        proc_ptr = next_proc;
    }

    free(job->command);
    free(job);
}

char *jobs_job_str(job_t *job, unsigned job_id) {
    shell_state_t *shell_state = shell_state_get();
    
    char *str_state;
    // stringify the job enum but (mannualy centered)
    switch(job->state) {
        case JOB_RUNNING: str_state = "running"; break;
        case JOB_DONE:    str_state = "  done "; break;
        case JOB_STOPPED: str_state = "stopped"; break;
        case JOB_KILLED:  str_state = " killed"; break;
        default: 
            return strdup("[Error: Unknown job state]\n"); 
    }

    // Determine active job character ('+', '-', or ' ')
    char active_job_char = ' ';
    
    // Check for the most recent job (+)
    if (shell_state->jobs_count > 0 && job_id == shell_state->jobs_count - 1) {
        active_job_char = '+'; 
    // Check for the second most recent job (-)
    } else if (shell_state->jobs_count > 1 && job_id == shell_state->jobs_count - 2) {
        active_job_char = '-'; 
    }

    char *buf = NULL;
    
    int result = asprintf(&buf, "[%d] %c %7s %s\n", 
                          job_id + 1,        // ID display (1-based)
                          active_job_char,   // Active status char
                          str_state,         // Aligned state string
                          job->command);        // Command string

    // Check if asprintf failed to allocate memory
    if (result < 0 || buf == NULL) {
        return strdup("[Error: asprintf failed to allocate memory]\n");
    }

    return buf;
}

char *jobs_last_job_str() {
    shell_state_t *shell_state = shell_state_get();
    if (shell_state->jobs_count == 0) return strdup("");

    job_t *job_ptr = shell_state->jobs;
    unsigned job_id = 0;
    while (job_ptr->next != NULL) {
        job_ptr = job_ptr->next;
        job_id++;
    }
    return jobs_job_str(job_ptr, job_id);
}