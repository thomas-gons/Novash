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

/**
 * allocate memory and exit on failure
 * @param s size of memory to allocate
 * @return pointer to allocated memory
 */
void *xmalloc(size_t size);

/**
 * allocate memory, initialize it to zero, and exit on failure
 * @param count The number of elements to allocate.
 * @param size The size of each element in bytes.
 * @return pointer to allocated, zero-initialized memory
 */
void *xcalloc(size_t count, size_t size);

/**
 * reallocate memory and exit on failure
 * @param ptr pointer to existing memory
 * @param s new size of memory
 * @return pointer to reallocated memory
 */
void *xrealloc(void *ptr, size_t size);

/**
 * duplicate a string and exit on failure
 * @param s string to duplicate
 * @return pointer to duplicated string
 */
char *xstrdup(const char *s);

/**
 * safe snprintf that exits on failure
 * @param buf buffer to write to
 * @param buf_sz size of the buffer
 * @param fmt format string
 * @param ... arguments for the format string
 * @return number of characters written
 */
int xsnprintf(char *buf, size_t buf_sz, const char *fmt, ...);


#endif // __MEMORY_H__