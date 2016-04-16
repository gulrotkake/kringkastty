#ifndef SGR_H
#define SGR_H

#include "buffer.h"
#include <stdint.h>

void sgr_sequence(uint32_t attribute, buffer *buffer);

#endif // SGR_H
