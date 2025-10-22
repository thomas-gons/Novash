/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "utils/memory.h"


void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size); 
    
    if (ptr == NULL) {
        perror("calloc failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *tmp_ptr = realloc(ptr, size);
    if (!tmp_ptr) {perror("realloc failed"); exit(EXIT_FAILURE);}
    return tmp_ptr;
}

char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t size = strlen(s);
    char *dup = xmalloc(size + 1);
    memcpy(dup, s, size);
    dup[size] = '\0';
    return dup;
}

int xsnprintf(char *buf, size_t buf_sz, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // Use vsnprintf to forward the variadic arguments from this wrapper
    // to the underlying snprintf implementation safely
    int needed = vsnprintf(buf, buf_sz, fmt, args);
    va_end(args);
    if (needed < 0) {
        perror("snprintf failed");
        exit(EXIT_FAILURE);
    }
    return needed;
}