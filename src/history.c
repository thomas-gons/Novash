#include "history.h"


static char *get_history_path() {
    shell_state_t *ss = shell_state_get();
    char *hist_file_path = hashmap_get(ss->environment, "HISTFILE");
    if (!hist_file_path) {
        fprintf(stderr, "error cannot resolve history file path"); 
        exit(EXIT_FAILURE);
    }
    return hist_file_path;
}

void history_init() {
    shell_state_t *ss = shell_state_get();
    history_t *hist = ss->hist;

    hist->cmd_list = xcalloc(sizeof(char*), HIST_SIZE);
    hist->timestamps = xcalloc(sizeof(time_t), HIST_SIZE);
    hist->cmd_count = 0;
    hist->start = 0;
}

void history_load() {
    char line[1024];

    shell_state_t *ss = shell_state_get();
    history_t *hist = ss->hist;
    
    hist->fp = fopen(get_history_path(), "a+");
    if (!hist->fp) {
        perror("fopen");
        exit(1);
    }
    rewind(hist->fp);

    while (fgets(line, sizeof(line), hist->fp)) {
        char *sep = strchr(line, ';');
        if (!sep) continue;
        *sep = '\0';
        time_t ts = atol(line + 2);
        char *cmd = sep + 1;
        cmd[strcspn(cmd, "\n")] = 0;

        if (hist->cmd_count < HIST_SIZE) {
            hist->cmd_list[hist->cmd_count] = xstrdup(cmd);
            hist->timestamps[hist->cmd_count] = ts;
            hist->cmd_count++;
        } else {
            free(hist->cmd_list[hist->start]);
            hist->cmd_list[hist->start] = xstrdup(cmd);
            hist->timestamps[hist->start] = ts;
            hist->start = (hist->start + 1) % HIST_SIZE;
        }
    }
}

void history_save_command(const char *cmd) {
    if (!cmd || !*cmd) return;

    time_t now = time(NULL);
    size_t idx;
    shell_state_t *shell_state = shell_state_get();
    history_t *hist = shell_state->hist;

    if (hist->cmd_count < HIST_SIZE) {
        idx = hist->cmd_count++;
    } else {
        idx = hist->start;
        free(hist->cmd_list[idx]);
        hist->start = (hist->start + 1) % HIST_SIZE;
    }

    hist->cmd_list[idx] = xstrdup(cmd);
    hist->timestamps[idx] = now;

    fprintf(hist->fp, "%ld;%s\n", now, cmd);
    fflush(hist->fp);
}

void history_trim() {
    shell_state_t *ss = shell_state_get();
    history_t *hist = ss->hist;

    if (hist->cmd_count < HIST_SIZE) {
        return;
    }

    FILE *hist_write_fp = fopen(get_history_path(), "w+");
    if (!hist_write_fp) { perror("open failed"); exit(1); }

    unsigned index = 0;
    unsigned hist_end = HIST_SIZE + hist->start;
    for (unsigned i = hist->start; i < hist_end; i++) {
        index = i % HIST_SIZE;
        fprintf(hist_write_fp, "%ld;%s\n", hist->timestamps[index], hist->cmd_list[index]);
    }
    fclose(hist_write_fp);
}

void history_free() {
    shell_state_t *ss = shell_state_get();
    history_t *hist = ss->hist;

    for (int i = 0; i < HIST_SIZE; i++) {
        if (hist->cmd_list[i] != NULL) {
            free(hist->cmd_list[i]);
        }
    }

    free(hist->cmd_list);
    free(hist->timestamps);
    free(hist);
}