#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdlib.h>

typedef struct {
  char *buffer;
  size_t used;
  size_t size;
} buffer;

void buf_init(buffer * a, size_t initial);
void buf_free(buffer *a);

size_t buf_len(buffer *a);
char * buf_get(buffer *a);

void buf_printf(buffer *a, const char * format, ...);
void buf_write(buffer *a, char *buffer, size_t len);

#endif
