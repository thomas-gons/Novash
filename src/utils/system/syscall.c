#include "syscall.h"

void xpipe(int pipefd[2]) {
  if (pipe(pipefd) == -1) {
    perror("pipe failed");
    exit(EXIT_FAILURE);
  }
}

void xdup2(int oldfd, int newfd, bool from_child) {
  if (dup2(oldfd, newfd) == -1) {
    if (from_child) {
      perror("dup2 failed in child");
      _exit(EXIT_CHILD_FAILURE);
    }
    perror("dup2 failed");
    exit(EXIT_FAILURE);
  }
}

pid_t xfork() {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
    exit(EXIT_FAILURE);
  }
  return pid;
}

int xopen(const char *pathname, int flags, mode_t mode, bool from_child) {
  int fd = open(pathname, flags, mode);
  if (fd == -1) {
    if (from_child) {
      perror("open failed in child");
      _exit(EXIT_CHILD_FAILURE);
    }
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  return fd;
}

void xclose(int fd) {
  if (close(fd) == -1) {
    perror("close failed");
    exit(EXIT_FAILURE);
  }
}

void xwaitpid(pid_t pid, int *status, int options) {
  if (waitpid(pid, status, options) == -1) {
    perror("waitpid failed");
    exit(EXIT_FAILURE);
  }
}

pid_t xgetpgid(pid_t pid) {
  pid_t pgid = getpgid(pid);
  if (pgid == -1) {
    perror("getpgid failed");
    exit(EXIT_FAILURE);
  }
  return pgid;
}

void xsetpgid(pid_t pid, pid_t pgid, bool from_child) {
  if (setpgid(pid, pgid) == -1) {
    if (from_child) {
      perror("setpgid failed in child");
      _exit(EXIT_CHILD_FAILURE);
    }
    perror("setpgid failed");
    exit(EXIT_FAILURE);
  }
}

void xwrite(int fd, const void *buf, size_t count) {
  ssize_t bytes_written = write(fd, buf, count);
  if (bytes_written == -1 || (size_t)bytes_written != count) {
    perror("write failed");
    exit(EXIT_FAILURE);
  }
}

void xsigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
  if (sigprocmask(how, set, oldset) == -1) {
    perror("sigprocmask failed");
    exit(EXIT_FAILURE);
  }
}

int xsignalfd(int fd, const sigset_t *mask, int flags) {
  int sfd = signalfd(fd, mask, flags);
  if (sfd == -1) {
    perror("signalfd failed");
    exit(EXIT_FAILURE);
  }
  return sfd;
}
