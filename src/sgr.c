#include "sgr.h"
#include "stdio.h"

static uint8_t get_mode(uint32_t mode) {
    switch(mode) {
    case 0x80000:
        return 1;
    case 0x100000:
        return 4;
    case 0x40000:
        return 5;
    case 0x200000:
        return 7;
    default:
        return 0;
    }
}

void sgr_sequence(uint32_t attribute, buffer *buffer) {
    unsigned i;
    unsigned int d = 0;
    uint32_t modes[4] = {0x80000, 0x100000, 0x40000, 0x200000};
    uint16_t color, highbit;
    buf_printf(buffer, "\033[");
    if (attribute == 0) {
        buf_printf(buffer, "0m");
        return;
    }

    for (i = 0; i<sizeof(modes)/sizeof(uint32_t); ++i) {
        if (attribute & modes[i]) {
            buf_printf(buffer, d++? ";%d" : "%d", get_mode(modes[i]));
        }
    }

    color = attribute & 0xFF;
    highbit = attribute & 0x100;
    printf("Color %d, high %d\n", color, highbit);
    if (highbit) {
        buf_printf(buffer, d++? ";38;5;%d" : "38;5;%d", color);
    } else if ((color >= 30 && color < 38) || color == 39) {
        buf_printf(buffer, d++? ";%d" : "%d", color);
    }

    color = (attribute >> 9) & 0xFF;
    highbit = (attribute >> 9) & 0x100;
    if (highbit) {
        buf_printf(buffer, d++? ";48;5;%d" : "48;5;%d", color);
    } else if ((color >= 40 && color < 48) || color == 49) {
        buf_printf(buffer, d++? ";%d" : "%d", color);
    }

    buf_printf(buffer, "m");
}
