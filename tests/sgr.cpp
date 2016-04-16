#include "tsetse.hpp"
#include "buffer.h"
#include <sgr.h>

static uint32_t SGR(unsigned int argc, unsigned int *args)
{
    unsigned int i = 0;
    uint32_t attr = 0;
    unsigned int argv;
    for (i = 0; i<argc; ++i) {
        argv = args[i];
        switch (argv) {
        case 0:
            attr = 0;
            break;
        case 1:
            attr |= 0x80000;
            break;
        case 4:
            attr |= 0x100000;
            break;
        case 5:
            attr |= 0x40000;
            break;
        case 7:
            attr |= 0x200000;
            break;
        case 38:
            if (i+2>=argc) goto fail;
            if (args[i+1] == 5) {
                attr |= ((uint16_t)(args[i+2]) & 0x0FF);
                attr |= 0x100;
            }
            break;
        case 48:
            if (i+2>=argc) goto fail;
            if (args[i+1] == 5) {
                attr |= ((uint16_t)(args[i+2]) & 0xFF) << 9;
                attr |= (0x100 << 9);
            }
            break;
        default:
            if (argv>30 && argv<38) {
                attr &= ~((uint32_t)0x1FF);
                attr |= ((uint16_t)(argv) & 0xFF);
            } if (argv>40 && argv<48) {
                attr &= ~(((uint32_t)0x1FF) << 9);
                attr |= ((uint16_t)(argv) & 0x1FF) << 9;
            }
        }
    }
    return attr;
fail:
    return 0;
}

static bool matches(const char *expected, buffer *buffer) {
    size_t elen = strlen(expected);
    size_t blen = buf_len(buffer);
    size_t idx;
    if (elen != blen)
        return false;
    for (idx = 0; idx<elen && idx<blen; ++idx) {
        if (expected[idx] != buf_get(buffer)[idx])
            return false;
    }
    return true;
}

TEST(lol) {
    unsigned int args[1024] = {0};
    args[0] = 38;
    args[1] = 5;
    args[2] = 44;
    uint32_t attr = SGR(3, args);
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(attr, &buffer);
    buf_write(&buffer, "\0", 1);
    printf("%s", buf_get(&buffer)+1);
    buf_free(&buffer);
}

TEST(intensity) {
    uint32_t intense = 0x80000;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(intense, &buffer);
    CHECK(matches("\033[1m", &buffer));
    buf_free(&buffer);
}

TEST(underscore) {
    uint32_t underscore = 0x100000;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(underscore, &buffer);
    CHECK(matches("\033[4m", &buffer));
    buf_free(&buffer);
}

TEST(blink) {
    uint32_t blink = 0x40000;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(blink, &buffer);
    CHECK(matches("\033[5m", &buffer));
    buf_free(&buffer);
}

TEST(inverse) {
    uint32_t inverse = 0x200000;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(inverse, &buffer);
    CHECK(matches("\033[7m", &buffer));
    buf_free(&buffer);
}

TEST(foreground) {
    uint32_t color = 35;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[35m", &buffer));
    buf_free(&buffer);
}

TEST(foreground_default) {
    uint32_t color = 39;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[39m", &buffer));
    buf_free(&buffer);
}

TEST(foreground_256) {
    uint32_t color = 0x50 | 0x100;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[38;5;80m", &buffer));
    buf_free(&buffer);
}

TEST(background) {
    uint32_t color = 45 << 9;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[45m", &buffer));
    buf_free(&buffer);
}

TEST(background_default) {
    uint32_t color = 49 << 9;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[49m", &buffer));
    buf_free(&buffer);
}

TEST(background_256) {
    uint32_t color = (0x50 | 0x100) << 9;
    buffer buffer;
    buf_init(&buffer, BUFSIZ);
    sgr_sequence(color, &buffer);
    CHECK(matches("\033[48;5;80m", &buffer));
    buf_free(&buffer);
}
