#ifndef __UTIL_H__
#define __UTIL_H__

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
void *xmalloc(size_t s);

/**
 * reallocate memory and exit on failure
 * @param ptr pointer to existing memory
 * @param s new size of memory
 * @return pointer to reallocated memory
 */
void *xrealloc(void *ptr, size_t s);

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

#endif // __UTIL_H__