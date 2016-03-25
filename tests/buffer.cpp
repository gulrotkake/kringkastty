#include "tsetse.hpp"
#include "buffer.h"

TEST(buffer_null_on_free) {
    buffer buf;
    buf_init(&buf, 1024);
    CHECK(buf.buffer != NULL);
    CHECK(buf.size == 1024);
    CHECK(buf.used == 0);

    buf_free(&buf);
    CHECK(buf.buffer == NULL);
}

TEST(buffer_printf) {
    buffer buf;
    buf_init(&buf, 1024);
    buf_printf(&buf, "a%s%d%c%c%c", "bc", 123, 'd', 'e', 'f');
    CHECK(buf.used == 9);
    const char *expected = "abc123def";
    size_t len = strlen(expected);
    for (int i = 0; i<len; ++i) {
        CHECK(buf.buffer[i] == expected[i]);
    }
    buf_free(&buf);
}

TEST(buffer_multiple_printf) {
    buffer buf;
    buf_init(&buf, 5);
    buf_printf(&buf, "%s", "abcd");
    buf_printf(&buf, "%s", "efgh");
    CHECK(buf.used == 8);
    const char *expected = "abcdefgh";
    size_t len = strlen(expected);
    for (int i = 0; i<len; ++i) {
        CHECK(buf.buffer[i] == expected[i]);
    }
    buf_free(&buf);
}

TEST(buffer_printf_grows) {
    buffer buf;
    buf_init(&buf, 5);
    CHECK(buf.size == 5);
    buf_printf(&buf, "%d", 12345);
    CHECK(buf.used == 5);
    CHECK(buf.size == 10);
    buf_free(&buf);
}

TEST(buffer_write) {
    buffer buf;
    buf_init(&buf, 10);
    CHECK(buf.size == 10);
    const char *expected = "abc123";
    buf_write(&buf, expected, 6);
    CHECK(buf.used == 6);
    for (int i = 0; i<6; ++i) {
        CHECK(buf.buffer[i] == expected[i]);
    }
    buf_free(&buf);
}

TEST(buffer_write_grows) {
    buffer buf;
    buf_init(&buf, 5);
    CHECK(buf.size == 5);
    const char *expected = "abc123";
    buf_write(&buf, expected, 3);
    buf_write(&buf, expected+3, 3);
    CHECK(buf.used == 6);
    CHECK(buf.size == 10);
    for (int i = 0; i<6; ++i) {
        CHECK(buf.buffer[i] == expected[i]);
    }
    buf_free(&buf);
}
