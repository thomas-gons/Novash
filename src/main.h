/*
 * Novash — a minimalist shell implementation.
 * Copyright (C) 2025 Thomas Gons
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


#define HIST_PATH ".nsh_history"
#define HIST_SIZE 10
#define MAX_JOBS 128

typedef int (*builtin_f_t) (int argc, char *argv[]);

typedef struct {
    char *name;
    builtin_f_t f;
} buitlin_t;

typedef enum {
    TOK_WORD,
    TOK_SEMI,
    TOK_PIPE,
    TOK_OR,
    TOK_AND,
    TOK_BG,
    TOK_FD,
    TOK_REDIR_IN,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND,
    TOK_HEREDOC,
    TOK_EOF,
} token_type_e;

typedef struct {
    token_type_e type;
    char *value;
} token_t;

typedef struct {
    char *input;
    unsigned pos;
    size_t length;
} tokenizer_t;

typedef enum { REDIR_IN, REDIR_OUT, REDIR_APPEND } redirection_e;

typedef struct {
    int fd;
    redirection_e type;
    char *target;
} redirection_t;

typedef struct {
    int argc;
    char **argv;
    redirection_t *redir;
    size_t redir_count;
    char *raw_str;
    bool is_bg;
} cmd_node_t;

typedef struct {
    struct ast_node_t *left;
    struct ast_node_t *right;
} pipe_node_t;

typedef enum { COND_AND, COND_OR } cond_op_e;

typedef struct {
    struct ast_node_t *left;
    struct ast_node_t *right;
    cond_op_e op;    
} cond_node_t;

typedef struct {
    struct ast_node_t **nodes;
    size_t nodes_count;
} seq_node_t;

typedef enum {
    NODE_CMD,
    NODE_PIPELINE,
    NODE_CONDITIONAL,
    NODE_SEQUENCE
} ast_node_type_e;

typedef struct ast_node_t {
    ast_node_type_e type;
    union {
        cmd_node_t cmd;
        pipe_node_t pipe;
        cond_node_t cond;
        seq_node_t seq;
    };
} ast_node_t;

typedef void (*runner_f_t) (cmd_node_t cmd_node);

bool is_metachar(char c);
void *realloc_s(void *ptr, size_t s);
char peek(tokenizer_t *tz);
char advance(tokenizer_t *tz);
void skip_whitespaces(tokenizer_t *tz);
token_t get_next_token(tokenizer_t *tz);
void print_token(token_t tok);

token_t g_tok;

void next_token(tokenizer_t *tz);
ast_node_t *parse_command(tokenizer_t *tz);
ast_node_t *parse_pipeline(tokenizer_t *tz);
ast_node_t *parse_conditional(tokenizer_t *tz);
ast_node_t *parse_sequence(tokenizer_t *tz);

int handle_redirection(cmd_node_t cmd_node);
void sigchld_handler(int sig);
int run_child(cmd_node_t cmd_node, runner_f_t runner_f);
void builtin_runner(cmd_node_t cmd_node);
void external_cmd_runner(cmd_node_t cmd_node);
int exec_node(ast_node_t *ast_node);

typedef struct {
    char *cmd_list[HIST_SIZE];
    time_t timestamps[HIST_SIZE];
    size_t cmd_count;
    unsigned start;
    FILE *fp;
} history_t;


typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } job_state_t;

char *state_job_str(job_state_t state);

typedef struct {
    pid_t pid;
    job_state_t state;
    char *cmd;
} job_t;

typedef struct {
    const char* path;
    history_t hist;
    job_t jobs[MAX_JOBS];
    size_t jobs_count;
    bool should_exit;
} shell_state_t;

shell_state_t shell_state = {0};


char *is_in_path(char *cmd);
bool is_builtin(char *name);
builtin_f_t get_builtin(char *name);

int cd_builtin(int argc, char *argv[]);
int echo_builtin(int argc, char *argv[]);
int exit_builtin(int argc, char *argv[]);
int jobs_builtin(int argc, char *argv[]);
int history_builtin(int argc, char *argv[]);
int pwd_builtin(int argc, char *argv[]);
int type_builtin(int argc, char *argv[]);

void load_history();
void save_cmd_to_history(const char *cmd);
void save_history();

#endif // __MAIN_H__