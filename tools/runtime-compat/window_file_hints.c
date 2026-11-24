#include <stddef.h>
#include <stdint.h>
#include <zoo.h>
#include "zoo_internal.h"

#include "../../third_party/libzoo/src/libzoo/zoo_window_classic.c"

static uint8_t captured_chars[25][80];
static uint8_t captured_colors[25][80];
static uint8_t expected_chars[25][80];
static uint8_t expected_colors[25][80];
static zoo_state state;
static zoo_text_window window;
static zoo_video_driver video;

void zoo_window_close(zoo_text_window *window) { (void) window; }

char *zoo_window_line_at(zoo_text_window *window, int pos) {
	(void) window;
	(void) pos;
	return NULL;
}

size_t strlen(const char *str) {
	size_t length = 0;
	while (str[length] != '\0') length++;
	return length;
}

char *strchr(const char *str, int value) {
	char match = (char) value;
	for (;;) {
		if (*str == match) return (char *) str;
		if (*str == '\0') return NULL;
		str++;
	}
}

static void capture_write(zoo_video_driver *driver, int16_t x, int16_t y, uint8_t color, uint8_t character) {
	(void) driver;
	if (x >= 0 && x < 80 && y >= 0 && y < 25) {
		captured_chars[y][x] = character;
		captured_colors[y][x] = color;
	}
}

static void reset_capture(void) {
	int x, y;
	for (y = 0; y < 25; y++) {
		for (x = 0; x < 80; x++) {
			captured_chars[y][x] = 0xCC;
			captured_colors[y][x] = 0xCC;
			expected_chars[y][x] = 0xCC;
			expected_colors[y][x] = 0xCC;
		}
	}
}

static void expected_write(int x, int y, uint8_t color, uint8_t character) {
	expected_chars[y][x] = character;
	expected_colors[y][x] = color;
}

static void expected_string(int x, int y, uint8_t color, const char *text) {
	int i;
	for (i = 0; text[i] != '\0'; i++) expected_write(x + i, y, color, (uint8_t) text[i]);
}

static int captured_matches_expected_row(int y) {
	int x, row;
	for (row = 0; row < 25; row++) {
		for (x = 0; x < 80; x++) {
			if (captured_chars[row][x] != expected_chars[row][x] ||
				captured_colors[row][x] != expected_colors[row][x]) return 0;
		}
	}
	(void) y;
	return 1;
}

int run_window_file_hint_checks(void) {
	reset_capture();
	window.viewing_file = true;
	window.line_pos = 0;
	window.line_count = 0;
	state.d_video = &video;
	video.func_write = capture_write;

	/* Pascal writes the empty side markers first, then overlays the two colored strings. */
	zoo_window_draw_line(&window, &state, -4, false);
	expected_write(7, 9, 0x1E, ' ');
	expected_write(51, 9, 0x1E, ' ');
	expected_string(7, 9, 0x1A, "   Use            to view text,");
	expected_string(14, 9, 0x1F, "\x18 \x19, Enter");
	if (!captured_matches_expected_row(9)) return 1;

	reset_capture();
	zoo_window_draw_line(&window, &state, -3, false);
	expected_write(7, 10, 0x1E, ' ');
	expected_write(51, 10, 0x1E, ' ');
	expected_string(8, 10, 0x1A, "                 to print.");
	expected_string(19, 10, 0x1F, "Alt-P");
	if (!captured_matches_expected_row(10)) return 2;

	return 0;
}
