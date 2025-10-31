/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "utils.h"

char *is_in_path(char *cmd) {
  char *dir;
  char *cmd_path = NULL;
  struct stat buf;

  char *path = shell_state_getenv("PATH");

  if (!path)
    return NULL;

  // Duplicate PATH string for strtok/strsep
  char *_p = xstrdup(path);

  for (char *p = _p; (dir = strsep(&p, ":")) != NULL;) {

    if (asprintf(&cmd_path, "%s/%s", dir, cmd) < 0) {
      free(_p);
      return NULL;
    }

    if (stat(cmd_path, &buf) == 0 && (buf.st_mode & S_IXUSR)) {
      free(_p);

      // Return a new copy of the full path. The original cmd_path must be
      // freed.
      char *result = xstrdup(cmd_path);
      free(cmd_path);
      return result;
    }

    // Failure for this directory: Free the path allocated by asprintf
    free(cmd_path);
    cmd_path = NULL;
  }

  // Global failure: Free the duplicated PATH string
  free(_p);
  return NULL;
}