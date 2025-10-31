/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "history.h"

static char *get_history_path() {
  char *hist_file_path = shell_state_getenv("HISTFILE");
  if (!hist_file_path) {
    fprintf(stderr, "error cannot resolve history file path");
    exit(EXIT_FAILURE);
  }
  return hist_file_path;
}

void history_init() {
  shell_state_t *ss = shell_state_get();
  history_t *hist = ss->hist;

  hist->cmd_list = xcalloc(sizeof(char *), HIST_SIZE);
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

  // Rewind to the start of the file before reading.
  rewind(hist->fp);

  while (fgets(line, sizeof(line), hist->fp)) {
    // Find separator ';'. Expected format: #<timestamp>;<command>\n
    char *sep = strchr(line, ';');
    if (!sep)
      continue;
    *sep = '\0';
    time_t ts = atol(line + 2);
    char *cmd = sep + 1;
    cmd[strcspn(cmd, "\n")] = 0;

    // --- Circular Buffer Logic ---
    if (hist->cmd_count < HIST_SIZE) {
      // Buffer not full: add linearly.

      hist->cmd_list[hist->cmd_count] = xstrdup(cmd);
      add_history(cmd);
      hist->timestamps[hist->cmd_count] = ts;
      hist->cmd_count++;
    } else {
      // Buffer full: overwrite the oldest element at 'start'.
      free(hist->cmd_list[hist->start]); // Free old string before overwrite.

      hist->cmd_list[hist->start] = xstrdup(cmd);
      hist->timestamps[hist->start] = ts;

      // Advance the start pointer (circularly).
      hist->start = (hist->start + 1) % HIST_SIZE;
    }
  }
}

void history_save_command(const char *cmd) {
  if (!cmd || !*cmd)
    return;

  time_t now = time(NULL);
  size_t idx;
  shell_state_t *shell_state = shell_state_get();
  history_t *hist = shell_state->hist;

  // Determine the storage index (idx).
  if (hist->cmd_count < HIST_SIZE) {
    idx = hist->cmd_count++;
  } else {
    // Buffer full: reuse the oldest slot (hist->start).
    idx = hist->start;
    free(hist->cmd_list[idx]);
    hist->start = (hist->start + 1) % HIST_SIZE;
  }

  // Store the new command in the in-memory buffer.
  hist->cmd_list[idx] = xstrdup(cmd);
  hist->timestamps[idx] = now;
  add_history(cmd);

  // Write command to the physical file.
  fprintf(hist->fp, "%ld;%s\n", now, cmd);
  // Ensure the data is immediately written to disk.
  fflush(hist->fp);
}

void history_trim() {
  shell_state_t *ss = shell_state_get();
  history_t *hist = ss->hist;

  // Only trim the file if the buffer is full (otherwise it's already aligned).
  if (hist->cmd_count < HIST_SIZE) {
    return;
  }

  // Reopen in write mode ('w+') to truncate the file.
  FILE *hist_write_fp = fopen(get_history_path(), "w+");
  if (!hist_write_fp) {
    perror("open failed");
    exit(1);
  }

  unsigned index = 0;
  // Iterate HIST_SIZE times, starting from the oldest command (hist->start).
  unsigned hist_end = HIST_SIZE + hist->start;
  for (unsigned i = hist->start; i < hist_end; i++) {
    // Circular traversal using modulo arithmetic.
    index = i % HIST_SIZE;
    fprintf(hist_write_fp, "%ld;%s\n", hist->timestamps[index],
            hist->cmd_list[index]);
  }
  fclose(hist_write_fp);
}

void history_free() {
  shell_state_t *ss = shell_state_get();
  history_t *hist = ss->hist;

  // Free each command string individually.
  for (int i = 0; i < HIST_SIZE; i++) {
    if (hist->cmd_list[i] != NULL) {
      free(hist->cmd_list[i]);
    }
  }

  free(hist->cmd_list);
  free(hist->timestamps);
  free(hist);
}