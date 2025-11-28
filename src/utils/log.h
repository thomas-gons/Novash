/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOG_BUF_SIZE 512

#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

#ifndef TEST
#define nsh_msg(fmt, ...)                                                      \
  {                                                                            \
    fprintf(stderr, "nsh: " fmt, ##__VA_ARGS__);                               \
  }
#else
#define nsh_msg(fmt, ...)                                                      \
  do {                                                                         \
  } while (0)
#endif

static inline void log_msg(const char *color, const char *level,
                           const char *file, int line, const char *fmt, ...) {
  char buf[MAX_LOG_BUF_SIZE];
  int n =
      snprintf(buf, sizeof(buf), "%s[%s] %s:%d: ", color, level, file, line);
  if (n < 0)
    return;

  va_list args;
  va_start(args, fmt);
  int m = vsnprintf(buf + n, sizeof(buf) - (size_t)n, fmt, args);
  va_end(args);

  if (m < 0)
    return;

  size_t total_len = (size_t)n + (size_t)m;
  if (total_len >= sizeof(buf))
    total_len = sizeof(buf) - 1;

  memcpy(buf + total_len, RESET, sizeof(RESET) - 1);
  total_len += sizeof(RESET) - 1;

  buf[total_len] = '\n';
  buf[total_len + 1] = '\0';

  fputs(buf, stderr);
}

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#if LOG_LEVEL >= LOG_LEVEL_ERR
#define pr_err(fmt, ...)                                                       \
  {                                                                            \
    log_msg(RED, "ERR", __FILE__, __LINE__, fmt, ##__VA_ARGS__);               \
    exit(EXIT_FAILURE);                                                        \
  }
#else
#define pr_err(fmt, ...)                                                       \
  do {                                                                         \
  } while (0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define pr_warn(fmt, ...)                                                      \
  log_msg(YELLOW, "WARN", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define pr_warn(fmt, ...)                                                      \
  do {                                                                         \
  } while (0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define pr_info(fmt, ...)                                                      \
  log_msg(BLUE, "INFO", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define pr_info(fmt, ...)                                                      \
  do {                                                                         \
  } while (0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define pr_debug(fmt, ...)                                                     \
  log_msg("\x1b[35m", "DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)                                                     \
  do {                                                                         \
  } while (0)
#endif

#endif // __LOG_H__
