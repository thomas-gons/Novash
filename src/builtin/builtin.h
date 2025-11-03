/*
 * Novash — minimalist shell
 * Copyright (C) 2025 Thomas Gons
 * Licensed under the GPLv3 or later.
 *
 * builtin.h — Definitions and declarations for internal shell commands.
 * Provides function mappings and interfaces for builtins such as
 * `cd`, `echo`, `exit`, and job control commands (`fg`, `bg`, `jobs`, etc.).
 */
#ifndef NOVASH_BUILTIN_H
#define NOVASH_BUILTIN_H

#include "shell/state.h"
#include "utils/collections.h"
#include "utils/system/syscall.h"
#include "utils/utils.h"
#include <stdbool.h>
#include <linux/limits.h>

/* Builtin function type */
typedef int (*builtin_t)(int argc, char *argv[]);

/* Hash table entry for builtins */
typedef struct {
  char *key;
  builtin_t value;
} builtin_entry_t;

void builtin_init();

/* builtin lookup */
bool builtin_is_builtin(char *name);
builtin_t builtin_get_function(char *name);

/* builtin commands */
int builtin_cd(int argc, char *argv[]);
int builtin_echo(int argc, char *argv[]);
int builtin_exit(int argc, char *argv[]);
int builtin_pwd(int argc, char *argv[]);
int builtin_type(int argc, char *argv[]);

int builtin_history(int argc, char *argv[]);

int builtin_jobs(int argc, char *argv[]);
int builtin_fg(int argc, char *argv[]);
int builtin_bg(int argc, char *argv[]);

#endif /* NOVASH_BUILTIN_H */
