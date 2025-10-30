#ifndef __XSYS_H__
#define __XSYS_H__

#define _GNU_SOURCE
#include <unistd.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define EXIT_CHILD_FAILURE 127


void xpipe(int pipefd[2]);
void xdup2(int oldfd, int newfd, bool from_child);
pid_t xfork();
void xclose(int fd);
void xwaitpid(pid_t pid, int *status, int options);
pid_t xgetpgid(pid_t pid);
void xsetpgid(pid_t pid, pid_t pgid, bool from_child);
void xwrite(int fd, const void *buf, size_t count);
void xsigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int xsignalfd(int fd, const sigset_t *mask, int flags);

#endif // __XSYS_H__