/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

 #include "executor.h"


/**
 * @brief Configures I/O redirection based on the command node's settings.
 * Closes the standard file descriptors (0, 1, 2) and opens new files
 * as specified in cmd_node.
 * @param cmd_node The command node with redirection information.
 * @return int 0 on success, -1 on failure (e.g., file not found).
 */
static int handle_redirection(redirection_t *redir) {
    int *fd = xmalloc(arr_len(redir) * sizeof(int));
    int oflag;
    for (unsigned i = 0; i < arr_len(redir); i++) {
        redirection_t r = redir[i];
    
        // Determine the open flags based on the redirection type.
        oflag = (r.type == REDIR_IN) ?  O_RDONLY:
                (r.type == REDIR_OUT) ? O_WRONLY | O_CREAT | O_TRUNC:
                                        O_WRONLY | O_CREAT | O_APPEND;

        // Open the target file. Permissions are 0644 (rw-r--r--).
        fd[i] = open(r.target, oflag, 0644);
        if (fd[i] == -1) { free(fd); perror("open failed"); return 1; }

        // Duplicate the newly opened FD onto the target FD (e.g., STDOUT_FILENO).
        if (dup2(fd[i], r.fd) == -1) { free(fd); perror("fd duplication failed"); return 1;}

        // Close the original FD, as the target FD now holds the reference.
        close(fd[i]);
    }

    free(fd);
    return 0;
}

/**
 * @brief Resets common signal handlers to their default behavior (previously modified by the shell).
 * Used in child processes to ensure they respond to signals like SIGSTOP.
 */
static inline void reset_signal_handlers() {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}


// Apply stdin/stdout if requested and the process's redirections
static void setup_child_io(process_t *proc, int in_fd, int out_fd) {
    if (in_fd != -1) dup2(in_fd, STDIN_FILENO);
    if (out_fd != -1) dup2(out_fd, STDOUT_FILENO);
    if (proc->redir) handle_redirection(proc->redir);
}

// Execute the process (builtin or external)
static void execute_process(process_t *proc) {
    if (builtin_is_builtin(proc->argv[0])) {
        builtin_f_t f = builtin_get_function(proc->argv[0]);
        int argc = (int) arr_len(proc->argv);
        _exit(f(argc, proc->argv));
    } else {
        char *path = is_in_path(proc->argv[0]);
        if (!path) {
            fprintf(stderr, "%s: command not found\n", proc->argv[0]);
            _exit(127);
        }
        execv(path, proc->argv);
        perror("exec failed");
        free(path);
        _exit(1);
    }
}

static pid_t fork_process(process_t *proc, pid_t pgid, int in_fd, int out_fd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        /* CHILD */
        int child_pid = getpid();

        if (pgid == 0) pgid = child_pid;

        if (setpgid(0, pgid) < 0) {
            perror("[child] setpgid");
        }

        reset_signal_handlers();

        if (in_fd != -1) {
            if (dup2(in_fd, STDIN_FILENO) < 0) perror("[child] dup2 in_fd");
        }
        if (out_fd != -1) {
            if (dup2(out_fd, STDOUT_FILENO) < 0) perror("[child] dup2 out_fd");
        }
        if (proc->redir) handle_redirection(proc->redir);


        execute_process(proc);
        /* never returns */
    }

    /* PARENT */
    if (pgid == 0) pgid = pid;
    if (setpgid(pid, pgid) < 0) {
        /* possible race, but try to report */
        perror("[parent] setpgid");
    }
    proc->pid = pid;

    return pid;
}

static int handle_pure_builtin_execution(process_t *proc) {
    int status = 0;
    int stdin_bak = dup(STDIN_FILENO);
    int stdout_bak = dup(STDOUT_FILENO);

    if (proc->redir) handle_redirection(proc->redir);

    builtin_f_t f = builtin_get_function(proc->argv[0]);
    status = f((int) arr_len(proc->argv), proc->argv);

    dup2(stdin_bak, STDIN_FILENO); close(stdin_bak);
    dup2(stdout_bak, STDOUT_FILENO); close(stdout_bak);

    return status;
}

static int handle_foreground_execution(job_t *job, pid_t pgid) {
    tcsetpgrp(STDIN_FILENO, pgid);

    bool stopped = false;
    int status = 0;
    process_t *proc = job->first_process;
    while (proc) {
        waitpid(proc->pid, &status, WUNTRACED);

        if (WIFSTOPPED(status)) {
            proc->stopped = true;
            stopped = true;
        } else {
            proc->completed = true;
            proc->status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        }

        proc = proc->next;
    }

    if (stopped) {
        job->state = JOB_STOPPED;
        jobs_add_job(job);
        printf("%s", jobs_last_job_str());
    } else {
        job->state = JOB_DONE;
    }

    tcsetpgrp(STDIN_FILENO, getpid());
    return status;
}

static int handle_background_execution(job_t *job, pid_t pgid) {
    jobs_add_job(job);
    printf("[%lu] %d\n", shell_state_get()->jobs_count, pgid);
    return 0;
}

static int run_job(job_t *job) {
    if (!job || !job->first_process) return -1;

    process_t *proc = job->first_process;
    pid_t pgid = 0;
    int in_fd = -1;
    int fd[2];

    // Single builtin without fork
    if (!job->is_background && !proc->next && builtin_is_builtin(proc->argv[0])) {
        return handle_pure_builtin_execution(proc);
    }

    // --- Fork each process in pipeline ---
    while (proc) {
        int out_fd = -1;
        if (proc->next) {
            if (pipe(fd) < 0) { perror("pipe"); return -1; }
            out_fd = fd[1];
        }

        pid_t pid = fork_process(proc, pgid, in_fd, out_fd);
        if (pgid == 0) pgid = pid;
        job->pgid = pgid;

        if (in_fd != -1) close(in_fd);
        if (out_fd != -1) close(out_fd);
        in_fd = proc->next ? fd[0] : -1;

        proc = proc->next;
    }

    if (job->is_background) {
        return handle_background_execution(job, pgid);
    }

    return handle_foreground_execution(job, pgid);
}


// Create a process from a command AST node and add it to the job
static void compile_command_job(ast_node_t *cmd_node, job_t *job) {
    if (!cmd_node || cmd_node->type != NODE_CMD) return;

    cmd_node_t *cmd = &cmd_node->cmd;

    job->command = cmd->raw_str ? xstrdup(cmd->raw_str) : xstrdup("<unknown>");

    // Warning: shallow copy of argv and redirections cause they belong
    // to the AST that lives during all the execution
    process_t *proc = jobs_new_process(cmd, true);
    job->is_background = cmd->is_bg;
    jobs_add_process_to_job(job, proc);
}

// Transform a pipeline AST node into a job with multiple processes
static void compile_pipeline_job(ast_node_t *node, job_t *job) {
    if (!node || node->type != NODE_PIPELINE) return;

    for (size_t i = 0; i < arr_len(node->pipe.nodes); i++) {
        ast_node_t *sub_node = node->pipe.nodes[i];
        compile_command_job(sub_node, job);
    }
}

int exec_node(ast_node_t *ast_node) {
    if (!ast_node) return -1;

    int status = 0;
    switch (ast_node->type) {

        case NODE_SEQUENCE: {
            seq_node_t seq = ast_node->seq;
            for (size_t i = 0; i < arr_len(seq.nodes); i++) {
                status = exec_node(seq.nodes[i]);
            }
            break;
        }

        case NODE_CONDITIONAL: {
            cond_node_t cond = ast_node->cond;
            status = exec_node(cond.left);
            if ((cond.op == COND_AND && status == 0) ||
                (cond.op == COND_OR  && status != 0)) {
                status = exec_node(cond.right);
            }
            break;
        }

        case NODE_PIPELINE: {
            job_t *job = jobs_new_job();
            compile_pipeline_job(ast_node, job);
            status = run_job(job);
            break;
        }

        case NODE_CMD: {
            job_t *job = jobs_new_job();
            compile_command_job(ast_node, job);
            status = run_job(job);
            break;
        }

        default:
            fprintf(stderr, "Unknown AST node type: %d\n", ast_node->type);
            return -1;
    }

    return status;
}
