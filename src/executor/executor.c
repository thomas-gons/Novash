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
  for (int i = 0; i < arrlen(redir); i++) {
    redirection_t r = redir[i];

    int oflag = (r.type == REDIR_IN)    ? O_RDONLY
                : (r.type == REDIR_OUT) ? (O_WRONLY | O_CREAT | O_TRUNC)
                                        : (O_WRONLY | O_CREAT | O_APPEND);

    int fd = xopen(r.target, oflag, 0644, true);

    if (dup2(fd, r.fd) == -1) {
      perror("dup2 failed");
      close(fd);
      return 1;
    }

    close(fd);
  }
  return 0;
}

// Execute the process (builtin or external)
static void execute_process(process_t *proc) {
  if (builtin_is_builtin(proc->argv[0])) {
    builtin_t f = builtin_get_function(proc->argv[0]);
    int argc = (int)arrlen(proc->argv);
    _exit(f(argc, proc->argv));
  } else {
    char *path = is_in_path(proc->argv[0]);
    if (!path) {
      fprintf(stderr, "%s: command not found\n", proc->argv[0]);
      _exit(EXIT_CHILD_FAILURE);
    }
    execv(path, proc->argv);
    perror("exec failed");
    free(path);
    _exit(EXIT_CHILD_FAILURE);
  }
}

static inline void executor_context_close_pipes(executor_ctx_t *ctx) {
  if (ctx->sync_pipe[0] != -1)
    close(ctx->sync_pipe[0]);
  if (ctx->sync_pipe[1] != -1)
    close(ctx->sync_pipe[1]);
}

static pid_t fork_process(process_t *proc, executor_ctx_t *ctx) {
  pid_t pid = xfork();
  if (pid > 0) {
    pr_info("Parent forked child '%s' with pid %d", proc->argv[0], pid);
    return pid;
  }

  /* CHILD */
  pr_info("Child process '%s' started (pid %d)", proc->argv[0], getpid());

  xsigprocmask(SIG_SETMASK, &ctx->prev_mask, NULL);
  close(ctx->sfd);

  if (ctx->pgid == 0) {
    close(ctx->sync_pipe[0]);
    xsetpgid(0, 0, true);
    (void)write(ctx->sync_pipe[1], "R", 1);
    close(ctx->sync_pipe[1]);
  } else {
    xsetpgid(0, ctx->pgid, true);
  }

  if (proc->redir && arrlen(proc->redir) > 0) {
    pr_info("Setting up redirections for '%s'", proc->argv[0]);
    handle_redirection(proc->redir);
  }

  if (ctx->in_fd != -1) {
    xdup2(ctx->in_fd, STDIN_FILENO, true);
    close(ctx->in_fd);
  }
  if (ctx->out_fd != -1) {
    xdup2(ctx->out_fd, STDOUT_FILENO, true);
    close(ctx->out_fd);
  }

  execute_process(proc);
  _exit(EXIT_FAILURE);
}

static int handle_pure_builtin_execution(process_t *proc) {

  int status = 0;
  int stdin_bak = dup(STDIN_FILENO);
  int stdout_bak = dup(STDOUT_FILENO);

  if (proc->redir)
    handle_redirection(proc->redir);

  pr_info("Executing pure builtin command '%s' in shell process",
          proc->argv[0]);
  builtin_t f = builtin_get_function(proc->argv[0]);
  status = f((int)arrlen(proc->argv), proc->argv);

  dup2(stdin_bak, STDIN_FILENO);
  close(stdin_bak);
  dup2(stdout_bak, STDOUT_FILENO);
  close(stdout_bak);
  return status;
}

int handle_foreground_execution(job_t *job, int sfd) {
  xtcsetpgrp(STDIN_FILENO, job->pgid);

  struct pollfd pfd;
  pfd.fd = sfd;
  pfd.events = POLLIN;

  while (job->live_processes > 0) {
    int rv = poll(&pfd, 1, -1);
    if (rv == -1) {
      if (errno == EINTR)
        continue;
      perror("poll");
      break;
    }
    if (pfd.revents & POLLIN) {
      /* drain pipe (non-blocking read) */
      struct signalfd_siginfo fdsi;
      ssize_t s;
      while ((s = read(sfd, &fdsi, sizeof(fdsi))) == sizeof(fdsi)) {
        switch (fdsi.ssi_signo) {
        case SIGCHLD:
          handle_sigchld_events();
          break;
        case SIGINT:
          if (job && job->pgid > 0)
            kill(-job->pgid, SIGINT);
          break;
        case SIGTSTP:
          if (job && job->pgid > 0)
            kill(-job->pgid, SIGTSTP);
          break;
        default: /* ignore */
          break;
        }
      }
      /* read returned < sizeof(fdsi) */
      if (s == -1 && errno != EAGAIN && errno != EINTR) {
        perror("read(sfd)");
      }

      if (job->state == JOB_STOPPED) {
        shell_regain_control();
        pr_info("Foreground job (pgid=%d) stopped — returning control to shell",
                (int)job->pgid);
        return JOB_STOPPED_EXIT_CODE;
      }
    }
  }
  jobs_remove_job(job->pgid);
  shell_regain_control();
  return 0;
}

static int handle_background_execution(job_t *job, pid_t pgid) {
  printf("[%zu] %d\n", job->id, pgid);
  return 0;
}

static executor_ctx_t executor_init_context(void) {
  executor_ctx_t ctx = {.pgid = 0,
                        .in_fd = -1,
                        .out_fd = -1,
                        .sync_pipe = {-1, -1},
                        .sfd = -1,
                        .mask = {{0}},
                        .prev_mask = {{0}}};

  // Synchronization pipe for first child
  xpipe(ctx.sync_pipe);

  sigemptyset(&ctx.mask);
  sigaddset(&ctx.mask, SIGCHLD);
  sigaddset(&ctx.mask, SIGINT);
  sigaddset(&ctx.mask, SIGTSTP);

  xsigprocmask(SIG_BLOCK, &ctx.mask, &ctx.prev_mask);

  ctx.sfd = xsignalfd(-1, &ctx.mask, SFD_NONBLOCK | SFD_CLOEXEC);
  return ctx;
}

static void executor_destroy_context(executor_ctx_t *ctx) {
  if (ctx->sfd != -1)
    close(ctx->sfd);
  if (ctx->sync_pipe[0] != -1)
    close(ctx->sync_pipe[0]);
  if (ctx->sync_pipe[1] != -1)
    close(ctx->sync_pipe[1]);
  xsigprocmask(SIG_SETMASK, &ctx->prev_mask, NULL);
}


static int run_job(job_t *job) {
  if (!job || !job->first_process)
    return -1;
    
  shell_reset_last_exec();
  shell_last_exec_t *last_exec = shell_state_get_last_exec();
  last_exec->command = xstrdup(job->command);

  jobs_add_job(job);
  process_t *proc = job->first_process;

  if (proc->next == NULL && builtin_is_builtin(proc->argv[0])) {
    // Pure builtin execution (no fork)
    // job's pgid and processes' pids are not set in this case
    int status = handle_pure_builtin_execution(proc);
    jobs_remove_job(0); // pgid is 0 for pure builtins
    return status;
  }

  executor_ctx_t ctx = executor_init_context();
  clock_gettime(CLOCK_MONOTONIC, &last_exec->started_at);
  while (proc) {

    int fd[2] = {-1, -1};
    ctx.out_fd = -1;

    if (proc->next) {
      xpipe(fd);
      ctx.out_fd = fd[1];
    }

    pid_t pid = fork_process(proc, &ctx);

    proc->pid = pid;
    proc->state = PROCESS_RUNNING;
    if (job->is_background) {
      last_exec->bg_pid = pid;
    }

    // Handle pgid for first child
    if (ctx.pgid == 0) {
      close(ctx.sync_pipe[1]);
      char c;
      ssize_t r;
      do {
        r = read(ctx.sync_pipe[0], &c, 1);
      } while (r == -1 && errno == EINTR);
      if (r <= 0)
        perror("read(sync)");
      close(ctx.sync_pipe[0]);
      ctx.pgid = getpgid(pid);
      job->pgid = ctx.pgid;
    } else {
      setpgid(pid, job->pgid); // best-effort
    }

    job->live_processes++;

    // Parent closes FDs not needed anymore
    if (ctx.in_fd != -1)
      close(ctx.in_fd);
    if (ctx.out_fd != -1)
      close(ctx.out_fd);

    ctx.in_fd = fd[0]; // next child reads from previous pipe
    proc = proc->next;
  }
  last_exec->pgid = job->pgid;

  int status;
  if (job->is_background)
    status = handle_background_execution(job, ctx.pgid);
  else
    status = handle_foreground_execution(job, ctx.sfd);
    
  executor_destroy_context(&ctx);
  clock_gettime(CLOCK_MONOTONIC, &last_exec->ended_at);
  last_exec->duration_ms = (double)(last_exec->ended_at.tv_sec - last_exec->started_at.tv_sec) * 1000.0 +
                           (double)(last_exec->ended_at.tv_nsec - last_exec->started_at.tv_nsec) / 1000000.0;
  
  last_exec->exit_status = status;
  return status;
}

// Create a process from a command AST node and add it to the job
static void compile_command_job(ast_node_t *cmd_node, job_t *job) {
  if (!cmd_node || cmd_node->type != NODE_CMD)
    return;

  cmd_node_t *cmd = &cmd_node->cmd;

  job->command = cmd->raw_str ? xstrdup(cmd->raw_str) : xstrdup("<unknown>");

  // Warning: shallow copy of argv and redirections cause they belong
  // to the AST that lives during all the execution
  process_t *proc = jobs_new_process(cmd, true);
  proc->parent_job = job;
  job->is_background = cmd->is_bg;
  jobs_add_process_to_job(job, proc);
}

// Transform a pipeline AST node into a job with multiple processes
static void compile_pipeline_job(ast_node_t *node, job_t *job) {
  if (!node || node->type != NODE_PIPELINE)
    return;

  for (int i = 0; i < arrlen(node->pipe.nodes); i++) {
    ast_node_t *sub_node = node->pipe.nodes[i];
    compile_command_job(sub_node, job);
  }
}

int exec_node(ast_node_t *ast_node) {
  if (!ast_node)
    return -1;

  int status = 0;
  switch (ast_node->type) {

  case NODE_SEQUENCE: {
    seq_node_t seq = ast_node->seq;
    for (int i = 0; i < arrlen(seq.nodes); i++) {
      status = exec_node(seq.nodes[i]);
    }
    break;
  }

  case NODE_CONDITIONAL: {
    cond_node_t cond = ast_node->cond;
    status = exec_node(cond.left);
    if ((cond.op == COND_AND && status == 0) ||
        (cond.op == COND_OR && status != 0)) {
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
