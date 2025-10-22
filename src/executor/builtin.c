/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "builtin.h"


static builtin_t builtins[] = {
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
    const char *home = shell_state_getenv("HOME");
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
    shell_state_t *shell_state = shell_state_get();
    shell_state->should_exit = true;
    return 0;
}


int builtin_jobs(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    job_t *jobs = shell_state->jobs;
    for (unsigned i = 0; i < shell_state->jobs_count; i++)
        printf("%s", job_str(jobs[i], i));
    
    return 0;
}

int builtin_history(int argc, char *argv[]) {
    shell_state_t *shell_state = shell_state_get();
    history_t *hist = shell_state->hist;
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
    shell_state_t *shell_state = shell_state_get();
    printf("%s\n", shell_state->cwd);
    return 0;
}

int builtin_type(int argc, char *argv[]) {
    if (argc < 2) {
        printf("type: missing argument\n");
        return 1;
    }

    char *cmd = argv[1];

    if (builtin_is_builtin(cmd)) {
        printf("%s is a shell builtin\n", cmd);
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