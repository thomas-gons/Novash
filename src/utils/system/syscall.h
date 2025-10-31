#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>  // for perror
#include <stdlib.h> // for exit
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT_CHILD_FAILURE 127

void xpipe(int pipefd[2]);
void xdup2(int oldfd, int newfd, bool from_child);
pid_t xfork();
int xopen(const char *pathname, int flags, mode_t mode, bool from_child);
void xclose(int fd);
void xwaitpid(pid_t pid, int *status, int options);
pid_t xgetpgid(pid_t pid);
void xsetpgid(pid_t pid, pid_t pgid, bool from_child);
void xwrite(int fd, const void *buf, size_t count);
void xsigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int xsignalfd(int fd, const sigset_t *mask, int flags);

#endif // __SYSCALL_H__