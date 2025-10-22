/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "jobs.h"


char *job_str(job_t job, unsigned job_id) {
    shell_state_t *shell_state = shell_state_get();
    
    char *str_state;
    // stringify the job enum but (mannualy centered)
    switch(job.state) {
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
                          job.cmd);          // Command string

    // Check if asprintf failed to allocate memory
    if (result < 0 || buf == NULL) {
        return strdup("[Error: asprintf failed to allocate memory]\n");
    }

    return buf;
}