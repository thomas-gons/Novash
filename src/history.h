#ifndef __HISTORY_H__
#define __HISTORY_H__

#define _GNU_SOURCE

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <shell_state.h>
#include "config.h"
#include "utils.h"


typedef struct {
    char *cmd_list[HIST_SIZE];
    time_t timestamps[HIST_SIZE];
    size_t cmd_count;
    unsigned start;
    FILE *fp;
} history_t;


char *get_history_path();
void load_history();
void save_cmd_to_history(const char *cmd);
void save_history();

#endif // __HISTORY_H__