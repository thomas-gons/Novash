/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __JOBS_H__
#define __JOBS_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include "parser/parser.h"
#include "shell/state.h"


typedef enum {
    JOB_RUNNING,
    JOB_DONE,
    JOB_STOPPED,
    JOB_KILLED
} job_state_e;

// Taken and adapted from GNU Bash's job structure

/* A process is a single process.  */
typedef struct process_t {
  pid_t pid;               /* process ID */
  char **argv;                /* for exec */
  redirection_t *redir;
  struct process_t *next;       /* next process in pipeline */
  bool completed;             /* true if process has completed */
  bool stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process_t;

/* A job is a pipeline of processes.  */
typedef struct job_t {
  pid_t pgid;                 /* process group ID */
  process_t *first_process;   /* list of processes in this job */
  char *command;              /* command line, used for messages */
  bool is_background;        /* true if job is a background job */
  job_state_e state;          /* job state */
  bool notified;              /* true if user told about stopped job */
  struct job_t *next;           /* next active job */
} job_t;


/**
 * @brief Creates a new process structure from a command node.
 * @param cmd Pointer to the command node (cmd_node_t).
 * @param deep_copy Flag indicating whether to deep copy argv and redirections.
 * @return process_t* Pointer to the newly created process structure.
 */
process_t *jobs_new_process(cmd_node_t *cmd, bool deep_copy);

/**
 * @brief Adds a process to the given job's process list.
 * @param job Pointer to the job structure.
 * @param process Pointer to the process structure to add.
 */
void jobs_add_process_to_job(job_t *job, process_t *process);

/**
 * @brief Frees the memory associated with a process.
 * @param process Pointer to the process structure to free.
 * @param deep_free Flag indicating whether to free argv and redirections deeply.
 */
void jobs_free_process(process_t *process, bool deep_free);

/**
 * @brief Creates a new job structure.
 * @return job_t* Pointer to the newly created job structure.
 */
job_t *jobs_new_job();

/**
 * @brief Adds a new job to the shell's job list.
 */
void jobs_add_job(job_t *job);

/**
 * @brief Removes a job from the shell's job list.
 * @param pgid The process group ID of the job to remove.
 * @return bool True if the job was found and removed, false otherwise.
 */
bool jobs_remove_job(pid_t pgid);

/**
 * @brief Retrieves a job by its process group ID.
 * @param pgid The process group ID of the job to retrieve.
 * @return job_t* Pointer to the job structure, or NULL if not found.
 */
job_t *jobs_get_job(pid_t pgid);


/**
 * @brief Frees the memory associated with a job.
 * @param job Pointer to the job structure to free.
 * @param deep_free Flag indicating whether to free associated processes deeply.
 */
void jobs_free_job(job_t *job, bool deep_free);

/**
 * @brief Generates a string representation of a job for display.
 * @param job Pointer to the job structure.
 * @param job_id The ID/index of the job in the job list.
 * @return char* A heap-allocated string containing the formatted job status.
*/
char *jobs_job_str(job_t *job, unsigned job_id);

/**
 * @brief Generates a string representation of the last job added.
 * @return char* A heap-allocated string containing the formatted last job status.
*/
char *jobs_last_job_str();

#endif // __JOBS_H__