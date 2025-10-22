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
#include "shell_state.h"
#include "config.h"
#include "utils.h"


typedef struct history_t {
    char **cmd_list;
    time_t *timestamps;
    size_t cmd_count;
    unsigned start;
    FILE *fp;
} history_t;


void history_init();
void history_load();
void history_save_command(const char *cmd);
void history_trim();
void history_free();

#endif // __HISTORY_H__