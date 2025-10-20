/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>
#include <time.h>
#include <readline/readline.h>
#include "tokenizer.h"
#include "parser.h"
#include "builtin.h"
#include "utils.h"


#define HIST_PATH ".nsh_history"
#define HIST_SIZE 10
#define MAX_JOBS 128

typedef int (*builtin_f_t) (int argc, char *argv[]);

typedef struct {
    char *name;
    builtin_f_t f;
} builtin_t;


bool builtin_is_builtin(char *name);
builtin_f_t builtin_get_function(char *name);

int builtin_cd(int argc, char *argv[]);
int builtin_echo(int argc, char *argv[]);
int builtin_exit(int argc, char *argv[]);
int builtin_jobs(int argc, char *argv[]);
int builtin_history(int argc, char *argv[]);
int builtin_pwd(int argc, char *argv[]);
int builtin_type(int argc, char *argv[]);

typedef void (*runner_f_t) (cmd_node_t cmd_node);


int handle_redirection(cmd_node_t cmd_node);
void sigint_handler(int sig);
void sigchld_handler(int sig);
int run_child(cmd_node_t cmd_node, runner_f_t runner_f);
void builtin_runner(cmd_node_t cmd_node);
void external_cmd_runner(cmd_node_t cmd_node);
int exec_pipeline(ast_node_t *ast_node);
int exec_node(ast_node_t *ast_node);

typedef struct {
    char *cmd_list[HIST_SIZE];
    time_t timestamps[HIST_SIZE];
    size_t cmd_count;
    unsigned start;
    FILE *fp;
} history_t;


typedef enum { JOB_RUNNING, JOB_DONE, JOB_STOPPED, JOB_KILLED } job_state_t;


typedef struct {
    pid_t pid;
    job_state_t state;
    char *cmd;
} job_t;

char *job_str(job_t job, unsigned job_id);


typedef struct {
    const char* path;
    history_t hist;
    job_t jobs[MAX_JOBS];
    size_t jobs_count;
    size_t running_jobs_count;
    bool should_exit;
} shell_state_t;

shell_state_t shell_state = {0};


char *is_in_path(char *cmd);

char *get_history_path();
void load_history();
void save_cmd_to_history(const char *cmd);
void save_history();

#endif // __MAIN_H__