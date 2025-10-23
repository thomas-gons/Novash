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
static int handle_redirection(cmd_node_t cmd_node) {
    int *fd = xmalloc(arr_len(cmd_node.redir) * sizeof(int));
    int oflag;
    for (unsigned i = 0; i < arr_len(cmd_node.redir); i++) {
        redirection_t redir = cmd_node.redir[i];
    
        // Determine the open flags based on the redirection type.
        oflag = (redir.type == REDIR_IN) ?  O_RDONLY:
                (redir.type == REDIR_OUT) ? O_WRONLY | O_CREAT | O_TRUNC:
                                                O_WRONLY | O_CREAT | O_APPEND;

        // Open the target file. Permissions are 0644 (rw-r--r--).
        fd[i] = open(redir.target, oflag, 0644);
        if (fd[i] == -1) { free(fd); perror("open failed"); return 1; }

        // Duplicate the newly opened FD onto the target FD (e.g., STDOUT_FILENO).
        if (dup2(fd[i], redir.fd) == -1) { free(fd); perror("fd duplication failed"); return 1;}

        // Close the original FD, as the target FD now holds the reference.
        close(fd[i]);
    }

    free(fd);
    return 0;
}


/**
 * @brief Function pointer type for command execution logic.
 * Used to run both builtins and external commands within a child process.
 * @param cmd_node The command node containing the name and arguments.
 */
typedef void (*runner_f_t) (cmd_node_t cmd_node);


/**
 * @brief Forks the process, sets up I/O, and executes the runner function in the child.
 * @param cmd_node The command node to execute.
 * @param runner_f The function pointer determining whether to run a builtin or external command.
 * @return int The PID of the newly created child process, or -1 on fork failure.
 */
static int run_child(cmd_node_t cmd_node, runner_f_t runner_f) {
    shell_state_t *shell_state = shell_state_get();
    pid_t pid = fork();

    if (pid == 0) {
        // --- Child Process ---
        // Put the child in its own process group (necessary for job control).
        setpgid(0, 0);
        if (!cmd_node.is_bg) {
            // Foreground job: give the terminal control to the child's process group.
            tcsetpgrp(STDIN_FILENO, getpid());
        } else {
            // Background job: redirect stdout/stderr to /dev/null to prevent output pollution.
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        // Handle I/O redirections before execution.
        if (handle_redirection(cmd_node) != 0) _exit(1);
        runner_f(cmd_node);
        _exit(0);
      } else if (pid > 0) {
        // --- Parent Process (Shell) ---
        // Put the child in its own process group in the parent too.
        setpgid(pid, pid);
        if (cmd_node.is_bg) {
            // Background handling: track job and return immediately.
            if (shell_state->jobs_count < MAX_JOBS) {
                shell_state->jobs[shell_state->jobs_count++] = (job_t) {
                    .pid = pid,
                    .state = JOB_RUNNING,
                    /* COPY the raw_str so job owns its string and it won't dangle
                       after parser_free_ast frees the AST's raw_str */
                    .cmd = xstrdup(cmd_node.raw_str),
                };
                shell_state->running_jobs_count++;
                printf("[%ld] %d\n", shell_state->jobs_count, pid);
            } else {
                fprintf(stderr, "Too many background tasks\n");
            }
        }
    } else {
        perror("fork failed");
        return -1;
    }
    return 0;
}

/**
 * @brief Execution logic for running a builtin command (only used in a child process for pipelines).
 * Calls the appropriate builtin function and exits the process with the return status.
 * @param cmd_node The command node containing the name and arguments.
 */
static void builtin_runner(cmd_node_t cmd_node) {
    builtin_f_t f = builtin_get_function(cmd_node.argv[0]);
    int r = f((int) arr_len(cmd_node.argv), cmd_node.argv);
    // Use _exit to terminate the child process immediately without calling cleanup handlers.
    _exit(r);
}

/**
 * @brief Execution logic for running an external command (via execvp).
 * Searches for the executable path and replaces the current process image.
 * @param cmd_node The command node containing the name and arguments.
 */
static void external_cmd_runner(cmd_node_t cmd_node) {
    // Attempt to find the full path of the command in PATH.
    char *cmd_path = is_in_path(cmd_node.argv[0]);
    if (!cmd_path) {
        fprintf(stderr, "%s: command not found\n", cmd_node.argv[0]);
        _exit(COMMAND_NOT_FOUND_EXIT_CODE); 
    }
    // execv replaces the current child process image with the new program.
    execv(cmd_path, cmd_node.argv); 
    
    // If execv returns, it failed. Print detailed error (e.g., permission denied).
    fprintf(stderr, "%s: %s\n", cmd_node.argv[0], strerror(errno));
    
    // Cleanup the path allocated by is_in_path() if execv failed.
    free(cmd_path); 
    _exit(1);
}

/**
 * @brief Determines if the command is a builtin or an external executable and runs it.
 * Builtins without redirection run in the current shell process; otherwise, a child process is forked.
 * External commands are always executed in a child process.
 * @param ast_node The AST node containing the command structure (cmd_node_t).
 * @return int The exit status of the executed command (0 on success).
 */
static int exec_command(ast_node_t *ast_node) {
    int r;
    cmd_node_t cmd_node = ast_node->cmd;
    char *cmd = ast_node->cmd.argv[0];

    if (!cmd) return 0;

    if (builtin_is_builtin(cmd)) {
        // Builtins must run in the main shell process if no redirection is present.
        if (arr_len(cmd_node.redir) == 0) {
            // Direct call to builtin function (no fork needed).
            builtin_f_t f = builtin_get_function(cmd);
            return f((int)arr_len(cmd_node.argv), cmd_node.argv);
        }
        // If redirection is present, builtins must be executed in a child process.
        return run_child(cmd_node, builtin_runner);
    }
    
    // Check if the external command exists in PATH.
    char *path = is_in_path(cmd);
    if (path != NULL) {
        // External command found: run in a new child process.
        r = run_child(cmd_node, external_cmd_runner);
        free(path);
    } else {
        printf("%s: command not found\n", cmd);
        r = COMMAND_NOT_FOUND_EXIT_CODE;
    }

    return r;
}

 /**
 * @brief Executes a sequence of commands connected by pipe operators (|).
 * Creates pipes, forks processes for each command, and connects their I/O.
 * @param ast_node The AST node representing the entire pipeline.
 * @return int The exit status of the *last* command in the pipeline.
 */
 static int exec_pipeline(ast_node_t *ast_node) {
    pipe_node_t pipe_node = ast_node->pipe;

    int fd[2];
    // Create a pipe: fd[0] is read end, fd[1] is write end.
    if (pipe(fd) < 0) {
        perror("pipe failed");
        return 1;
    }

    // --- Left-hand side (writer) ---
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Redirect standard output to the write end of the pipe.
        dup2(fd[1], STDOUT_FILENO); 
        // Close both pipe FDs in the child before exec.
        close(fd[0]);
        close(fd[1]);
        // Recursively execute the left command/sub-tree.
        exec_node(pipe_node.left);
        _exit(1);
    }

    // --- Right-hand side (reader) ---
    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Redirect standard input to the read end of the pipe.
        dup2(fd[0], STDIN_FILENO);
        // Close both pipe FDs in the child before exec.
        close(fd[0]);
        close(fd[1]);
        // Recursively execute the right command/sub-tree.
        exec_node(pipe_node.right);
        _exit(1);
    }

    // --- Parent (Shell) ---
    // The parent must close both ends of the pipe to ensure EOF is signalled.
    close(fd[0]);
    close(fd[1]);

    int status;
    // Wait for both children to finish.
    waitpid(pid1, NULL, 0);
    waitpid(pid2, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

int exec_node(ast_node_t *ast_node) {
    if (!ast_node)
        return -1;

    int r = 0;
    switch (ast_node->type) {
        case NODE_SEQUENCE:
            seq_node_t seq_node = ast_node->seq;
            // Execute nodes sequentially, updating the exit status 'r' each time.
            for (unsigned i = 0; i < arr_len(seq_node.nodes); i++) {
                r = exec_node(seq_node.nodes[i]);
            }
            return r;

        case NODE_CONDITIONAL:
            cond_node_t cond_node = ast_node->cond;
            if (cond_node.op == COND_AND) {
                // Logical AND (&&): execute right side ONLY if left side succeeded (r == 0).
                r = exec_node(cond_node.left);
                if (r == 0) r = exec_node(cond_node.right);
            } else if (cond_node.op == COND_OR) {
                // Logical OR (||): execute right side ONLY if left side failed (r != 0).
                r = exec_node(cond_node.left);
                if (r != 0) r = exec_node(cond_node.right);
            }
            return r;

        case NODE_PIPELINE:
            return exec_pipeline(ast_node);

        case NODE_CMD:
            return exec_command(ast_node);

        default:
            fprintf(stderr, "Unknown AST node type: %d\n", ast_node->type);
            return -1;
    }
}