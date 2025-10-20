#include "utils.h"

void *xmalloc(size_t s) {
    void *ptr = malloc(s);
    if (!ptr) {perror("malloc failed"); exit(EXIT_FAILURE);}
    return ptr;
}

void *xrealloc(void *ptr, size_t s) {
    void *tmp_ptr = realloc(ptr, s);
    if (!tmp_ptr) {perror("realloc failed"); exit(EXIT_FAILURE);}
    return tmp_ptr;
}

char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t sz = strlen(s);
    char *dup = xmalloc(sz + 1);
    memcpy(dup, s, sz);
    dup[sz] = '\0';
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