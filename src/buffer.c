#include <buffer.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void check_size(buffer *a, size_t needed) {
    int ns = a->used + needed;
    if (ns > a->size) {
        a->size = ns>a->size*2
            ? ns
            : a->size*2;
        a->buffer = realloc(a->buffer, a->size * sizeof(char));
    }
}

size_t buf_len(buffer *a) {
    return a->used;
}

char * buf_get(buffer *a) {
    return a->buffer;
}

void buf_init(buffer *a, size_t initial) {
    a->buffer = malloc(initial * sizeof(char));
    a->used = 0;
    a->size = initial;
}

void buf_printf(buffer *a, const char * restrict format, ...) {
    va_list ap, ap2;
    int needed;

    va_start(ap, format);
    va_copy(ap2, ap);
    needed = vsnprintf(NULL, 0, format, ap) + 1; // Make space for the null byte
    va_end(ap);

    check_size(a, needed);

    va_start(ap2, format);
    a->used += vsnprintf(a->buffer+a->used, needed + 1, format, ap2);
    va_end(ap2);
}

void buf_write(buffer *a, char *buffer, size_t len) {
    check_size(a, len);
    memcpy(a->buffer+a->used, buffer, len);
    a->used += len;
}

void buf_free(buffer *a) {
    free(a->buffer);
    a->buffer = NULL;
    a->used = a->size = 0;
}
