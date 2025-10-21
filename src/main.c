/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "main.h"


int exec_pipeline(ast_node_t *ast_node) {
    pipe_node_t pipe_node = ast_node->pipe;

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_node(pipe_node.left);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_node(pipe_node.right);
        _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return 0;
}

int handle_redirection(cmd_node_t cmd_node) {
    int *fd = xmalloc(cmd_node.redir_count * sizeof(int));
    int oflag;
    for (unsigned i = 0; i < cmd_node.redir_count; i++) {
        redirection_t redir = cmd_node.redir[i];
        oflag = (redir.type == REDIR_IN) ?  O_RDONLY:
                    (redir.type == REDIR_OUT) ? O_WRONLY | O_CREAT | O_TRUNC:
                                                O_WRONLY | O_CREAT | O_APPEND;
        
        fd[i] = open(redir.target, oflag, 0644);
        if (fd[i] == -1) { perror("open failed"); return 1; }
        if (dup2(fd[i], redir.fd) == -1) { perror("fd duplication failed"); return 1;}

        close(fd[i]);
    }
    return 0;
}

void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n$ ", 3);
    fflush(stdout);
}

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    char *buf;
    job_t *jobs = shell_state.jobs;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        unsigned job_id = 0;
        for (unsigned i = 0; i < shell_state.jobs_count; i++) {
            if (jobs[i].pid == pid) { job_id = i; break; }
        }
        job_t *job = &jobs[job_id];
        if (WIFSTOPPED(status)) {
            job->state = JOB_STOPPED;
        } else if (WIFSIGNALED(status)) {
            job->state = JOB_KILLED;
        } else {
            job->state = JOB_DONE;
        }
        shell_state.running_jobs_count--;
        buf = job_str(*job, job_id);
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, "$ ", 2);
    }
}

int run_child(cmd_node_t cmd_node, runner_f_t runner_f) {
    pid_t pid = fork();

    if (pid == 0) {
        setpgid(0, 0);
        if (!cmd_node.is_bg) {
            tcsetpgrp(STDIN_FILENO, getpid());
        } else {
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        if (handle_redirection(cmd_node) != 0) _exit(1);
        runner_f(cmd_node);
        _exit(0);
    } else if (pid > 0) {
        setpgid(pid, pid);
        if (cmd_node.is_bg) {
            if (shell_state.jobs_count < MAX_JOBS) {
                shell_state.jobs[shell_state.jobs_count++] = (job_t) {
                    .pid = pid,
                    .state = JOB_RUNNING,
                    .cmd=cmd_node.raw_str,
                };
                shell_state.running_jobs_count++;
                printf("[%ld] %d\n", shell_state.jobs_count, pid);
            } else {
                fprintf(stderr, "Too many background tasks\n");
            }
            return 0;
        }
        int status;
        tcsetpgrp(STDIN_FILENO, pid);
        waitpid(pid, &status, 0);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    } else {
        perror("fork failed");
        return -1;
    }
}

void builtin_runner(cmd_node_t cmd_node) {
    builtin_f_t f = builtin_get_function(cmd_node.argv[0]);
    int r = f(cmd_node.argc, cmd_node.argv);
    _exit(r);
}

void external_cmd_runner(cmd_node_t cmd_node) {
    char *cmd_path = is_in_path(cmd_node.argv[0]);
    if (!cmd_path) {
        fprintf(stderr, "%s: command not found\n", cmd_node.argv[0]);
        _exit(127);
    }
    execv(cmd_path, cmd_node.argv);
    fprintf(stderr, "%s: %s\n", cmd_node.argv[0], strerror(errno));
    free(cmd_path);
    _exit(1);
}

int exec_node(ast_node_t *ast_node) {
    if (!ast_node)
        return -1;

    int r = 0;
    switch (ast_node->type) {
        case NODE_SEQUENCE:
            seq_node_t seq_node = ast_node->seq;
            for (unsigned i = 0; i < seq_node.nodes_count; i++) {
                r = exec_node(seq_node.nodes[i]);
            }
            return r;

        case NODE_CONDITIONAL:
            cond_node_t cond_node = ast_node->cond;
            if (cond_node.op == COND_AND) {
                r = exec_node(cond_node.left);
                if (r == 0) r = exec_node(cond_node.right);
            } else if (cond_node.op == COND_OR) {
                r = exec_node(cond_node.left);
                if (r != 0) r = exec_node(cond_node.right);
            }
            return r;

        case NODE_PIPELINE:
            return exec_pipeline(ast_node);

        case NODE_CMD:
            cmd_node_t cmd_node = ast_node->cmd;
            char *cmd = ast_node->cmd.argv[0];

            if (!cmd) return 0;

            if (builtin_is_builtin(cmd)) {
                if (cmd_node.redir_count == 0) {
                    builtin_f_t f = builtin_get_function(cmd);
                    return f(cmd_node.argc, cmd_node.argv);
                }
                return run_child(cmd_node, builtin_runner);
            }
            
            if ((is_in_path(cmd)) != NULL) {
                return run_child(cmd_node, external_cmd_runner);
            } else {
                printf("%s: command not found\n", cmd);
                r = 127;
            }

            return r;

        default:
            fprintf(stderr, "Unknown AST node type: %d\n", ast_node->type);
            return -1;
    }
}


char *is_in_path(char *cmd) {
    char *dir;
    char *cmd_path;
    struct stat buf;

    char *_p = xstrdup(shell_state.path);

    for (char *p = _p; (dir = strsep(&p, ":")) != NULL;) {
        asprintf(&cmd_path, "%s/%s", dir, cmd);
        if (stat(cmd_path, &buf) == 0 && buf.st_mode & S_IXUSR) {
            free(_p);
            return xstrdup(cmd_path);
        }
    }

    free(_p);
    return NULL;
}


builtin_t builtins[] = {
    {"cd", builtin_cd},
    {"echo", builtin_echo},
    {"exit", builtin_exit},
    {"jobs", builtin_jobs},
    {"history", builtin_history},
    {"pwd", builtin_pwd},
    {"type", builtin_type},
};

#define BUILTINS_N (int) (sizeof(builtins) / sizeof(builtin_t))

int builtin_cd(int argc, char *argv[]) {
    const char *home = getenv("HOME");
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
    shell_state.should_exit = true;
    return 0;
}

char *job_str(job_t job, unsigned job_id) {
    char *str_state;
    switch(job.state) {
        // centering state string
        case JOB_RUNNING: str_state = "running"; break;
        case JOB_DONE:    str_state = "  done "; break;
        case JOB_STOPPED: str_state = "stopped"; break;
        case JOB_KILLED:  str_state = " killed"; break;
        default: return "unknown"; break;
    }

    char *buf;
    char active_job_char = (job_id == shell_state.jobs_count - 1) ? '+' :
                       ((job_id == shell_state.jobs_count - 2) ? '-' : ' ');


    asprintf(&buf, "[%d] %c %7s %s\n", job_id + 1, active_job_char, str_state, job.cmd);
    return buf;
}

int builtin_jobs(int argc, char *argv[]) {
    job_t *jobs = shell_state.jobs;
    for (unsigned i = 0; i < shell_state.jobs_count; i++)
        printf("%s", job_str(jobs[i], i));
    
    return 0;
}

int builtin_history(int argc, char *argv[]) {
    history_t *hist = &shell_state.hist;
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
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    return 0;
}

int builtin_type(int argc, char *argv[]) {
    if (argc < 2) {
        printf("type: missing argument\n");
        return 1;
    }

    char *cmd = argv[1];

    if (builtin_is_builtin(cmd)) {
        printf("%s is a shell_state builtin\n", cmd);
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

char *get_history_path() {
    char exe[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len == -1) return xstrdup(HIST_PATH);
    exe[len] = '\0';

    char *dir = dirname(exe);

    char *last = strrchr(dir, '/');
    if (last && strcmp(last + 1, "build") == 0) *last = '\0';

    char *path;
    asprintf(&path, "%s/%s", dir, HIST_PATH);
    return path;
}

void load_history() {
    char line[1024];

    history_t *hist = &shell_state.hist;
    
    hist->fp = fopen(get_history_path(), "a+");
    if (!hist->fp) {
        perror("fopen");
        exit(1);
    }
    rewind(hist->fp);

    while (fgets(line, sizeof(line), hist->fp)) {
        char *sep = strchr(line, ';');
        if (!sep) continue;
        *sep = '\0';
        time_t ts = atol(line + 2);
        char *cmd = sep + 1;
        cmd[strcspn(cmd, "\n")] = 0;

        if (hist->cmd_count < HIST_SIZE) {
            hist->cmd_list[hist->cmd_count] = xstrdup(cmd);
            hist->timestamps[hist->cmd_count] = ts;
            hist->cmd_count++;
        } else {
            free(hist->cmd_list[hist->start]);
            hist->cmd_list[hist->start] = xstrdup(cmd);
            hist->timestamps[hist->start] = ts;
            hist->start = (hist->start + 1) % HIST_SIZE;
        }
    }
}

void save_cmd_to_history(const char *cmd) {
    if (!cmd || !*cmd) return;

    time_t now = time(NULL);
    size_t idx;
    history_t *hist = &shell_state.hist;

    if (hist->cmd_count < HIST_SIZE) {
        idx = hist->cmd_count++;
    } else {
        idx = hist->start;
        free(hist->cmd_list[idx]);
        hist->start = (hist->start + 1) % HIST_SIZE;
    }

    hist->cmd_list[idx] = xstrdup(cmd);
    hist->timestamps[idx] = now;

    fprintf(hist->fp, "%ld;%s\n", now, cmd);
    fflush(hist->fp);
}

void save_history() {
    history_t *hist = &shell_state.hist;

    if (hist->cmd_count < HIST_SIZE) {
        return;
    }

    FILE *hist_write_fp = fopen(get_history_path(), "w+");
    if (!hist_write_fp) { perror("open failed"); exit(1); }

    unsigned index = 0;
    unsigned hist_end = HIST_SIZE + hist->start;
    for (unsigned i = hist->start; i < hist_end; i++) {
        index = i % HIST_SIZE;
        fprintf(hist_write_fp, "%ld;%s\n", hist->timestamps[index], hist->cmd_list[index]);
    }
    fclose(hist_write_fp);
}

int main() {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    tcsetpgrp(STDIN_FILENO, getpgrp());
    shell_state.path = getenv("PATH");
    //load_history();

    setbuf(stdout, NULL);
    tokenizer_t *tz = tokenizer_new();
    char *input = NULL;

    bool warning_exit = false;
    do {
        input = readline("$ ");
        if (input == NULL) {
            if (shell_state.running_jobs_count > 0 && !warning_exit) {
                printf("you have running jobs\n");
                warning_exit = true;
                continue;
            } else {
                printf("exit\n");
                break;
            }
        }

        warning_exit = false;
        tokenizer_init(tz, input);
            
        ast_node_t *ast_node = parser_create_ast(tz);
        //save_cmd_to_history(tz->input);

        //exec_node(ast_node);
        parser_free_ast(ast_node);
        free(input);
        
    } while (!shell_state.should_exit);
    tokenizer_free(tz);
    save_history();
    return 0;
}