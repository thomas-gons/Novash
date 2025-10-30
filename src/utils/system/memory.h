/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


// All the wrappers around memory allocation follow the OOM handling policy
// they exit the program if allocation fails

void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
int xsnprintf(char *buf, size_t buf_sz, const char *fmt, ...);


#endif // __MEMORY_H__