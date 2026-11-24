#include <stddef.h>
#include <stdint.h>
#include <zoo.h>
#include <string.h>

#include "../../third_party/libzoo/src/libzoo/zoo_window_classic.c"

void zoo_window_close(zoo_text_window *window) { (void) window; }
bool zoo_window_open_file(zoo_io_driver *io, zoo_text_window *window, const char *filename) {
    (void) io; (void) window; (void) filename; return false;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *) dest;
    const uint8_t *s = (const uint8_t *) src;
    size_t i;
    for (i = 0; i < count; i++) d[i] = s[i];
    return dest;
}

void *memset(void *dest, int value, size_t count) {
    uint8_t *d = (uint8_t *) dest;
    size_t i;
    for (i = 0; i < count; i++) d[i] = (uint8_t) value;
    return dest;
}

size_t strlen(const char *str) {
    size_t length = 0;
    while (str[length] != '\0') length++;
    return length;
}

char *strncpy(char *dest, const char *src, size_t count) {
    size_t i;
    for (i = 0; i < count && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < count; i++) dest[i] = '\0';
    return dest;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) { left++; right++; }
    return (uint8_t) *left - (uint8_t) *right;
}

char *strchr(const char *str, int value) {
    char match = (char) value;
    for (;;) {
        if (*str == match) return (char *) str;
        if (*str == '\0') return NULL;
        str++;
    }
}

static bool hyperlink(zoo_text_window *window, char *line) {
    zoo_state unused_state;
    return zoo_window_hyperlink(window, &unused_state, line);
}

int run_window_label_checks(void) {
    zoo_text_window window;
    char *lines[3];
    char target[] = "!mIxEd;Open section";
    char prefix_target[] = "!prefix;Open section";
    char missing_target[] = "!absent;Open section";
    char *document[] = {":Start;Top", ":MiXeD;Target", ":PrefixRoom;Prefix target"};

    memset(&window, 0, sizeof(window));
    lines[0] = document[0]; lines[1] = document[1]; lines[2] = document[2];
    window.lines = lines;
    window.line_count = 3;

    if (hyperlink(&window, target) || window.line_pos != 1) return 1;
    if (hyperlink(&window, prefix_target) || window.line_pos != 2) return 2;
    if (!hyperlink(&window, missing_target) || window.line_pos != 2) return 3;

    window.hyperlink_as_select = true;
    if (!hyperlink(&window, target) || strcmp(window.hyperlink, "mIxEd") != 0) return 4;
    return 0;
}
