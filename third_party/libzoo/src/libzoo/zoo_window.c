/**
 * Copyright (c) 2020 Adrian Siekierka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "zoo_internal.h"

#define ZOO_WINDOW_LINE_MAX 50

char *zoo_window_line_at(zoo_text_window *window, int pos) {
	if (pos >= 0 && pos < window->line_count) {
		return window->lines[pos];
	} else {
		return NULL;
	}
}

char *zoo_window_line_selected(zoo_text_window *window) {
	return zoo_window_line_at(window, window->line_pos);
}

void zoo_window_append(zoo_text_window *window, const char *text) {
	char *buffer;
	int16_t buflen = strlen(text);
	if (buflen > ZOO_WINDOW_LINE_MAX) buflen = ZOO_WINDOW_LINE_MAX;

	buffer = malloc(sizeof(char) * (buflen + 1));
	memcpy(buffer, text, buflen);
	buffer[buflen] = '\0';

	if (window->line_count > 0) {
		window->lines = realloc(window->lines, sizeof(char*) * (window->line_count + 1));
	} else {
		window->lines = malloc(sizeof(char*));
	}
	window->lines[(window->line_count)++] = buffer;
}

void zoo_window_append_pascal(zoo_state *state, zoo_text_window *window, const char *text) {
	char *record = NULL;
	uint32_t offset = 0;
	int16_t buflen = strlen(text);
	int16_t line_index = window->line_count < 0 ? 0 : window->line_count;
	int16_t needed = line_index + 1;

	if (buflen > ZOO_WINDOW_LINE_MAX) buflen = ZOO_WINDOW_LINE_MAX;
	if (state != NULL && state->board.data_heap != NULL) {
		if (window->pascal_heap_line_capacity < needed) {
			int16_t new_capacity = window->pascal_heap_line_capacity > 0
				? window->pascal_heap_line_capacity : 4;
			char **new_records;
			while (new_capacity < needed) new_capacity *= 2;
			new_records = (char **) realloc(window->pascal_heap_line_records,
				sizeof(char *) * (size_t) new_capacity);
			if (new_records != NULL) {
				memset(new_records + window->pascal_heap_line_capacity, 0,
					sizeof(char *) * (size_t) (new_capacity - window->pascal_heap_line_capacity));
				window->pascal_heap_line_records = new_records;
				window->pascal_heap_line_capacity = new_capacity;
			}
		}
		if (window->pascal_heap_line_capacity >= needed) {
			window->pascal_heap_line_records[line_index] = NULL;
			record = zoo_board_data_heap_alloc(&state->board, ZOO_WINDOW_LINE_MAX + 1, &offset);
			if (record != NULL) {
				record[0] = (char) buflen;
				memcpy(record + 1, text, (size_t) buflen);
				zoo_board_data_heap_mark_allocated(&state->board, offset, ZOO_WINDOW_LINE_MAX + 1);
				window->pascal_heap_line_records[line_index] = record;
				window->pascal_heap_board = &state->board;
			}
		}
	}

	zoo_window_append(window, text);
}

void zoo_window_close(zoo_text_window *window) {
	int16_t i;

	for (i = 0; i < window->line_count; i++) {
		free(window->lines[i]);
	}
	free(window->lines);
	if (window->pascal_heap_board != NULL && window->pascal_heap_line_records != NULL) {
		for (i = window->line_count - 1; i >= 0; i--) {
			if (i < window->pascal_heap_line_capacity
				&& window->pascal_heap_line_records[i] != NULL) {
				zoo_board_data_heap_release(window->pascal_heap_board,
					window->pascal_heap_line_records[i], 0);
			}
		}
	}
	free(window->pascal_heap_line_records);

	window->line_pos = 0;
	window->line_count = 0;
	window->pascal_heap_line_records = NULL;
	window->pascal_heap_line_capacity = 0;
	window->pascal_heap_board = NULL;
}

void zoo_window_append_file(zoo_text_window *window, zoo_io_handle *h) {
	char str[ZOO_WINDOW_LINE_MAX + 1];
	char c;
	int16_t i = 0;

	while (h->func_read(h, (uint8_t*) &c, 1) > 0) {
		if (c == '\x0D') {
			str[i++] = '\0';
			zoo_window_append(window, str);
			i = 0;
		} else if (c != '\x0A' && i < ZOO_WINDOW_LINE_MAX) {
			str[i++] = c;
		}
	}

	str[i++] = '\0';
	zoo_window_append(window, str);
}

bool zoo_window_open_file(zoo_io_driver *io, zoo_text_window *window, const char *filename) {
	char path[ZOO_WINDOW_LINE_MAX];
	zoo_io_handle h;

	strncpy(path, filename, sizeof(path));
	if (strchr(filename, '.') == NULL) {
		strncat(path, ".HLP", sizeof(path) - 1);
	}

	h = io->func_open_file(io, path, MODE_READ);
	zoo_window_append_file(window, &h);
	h.func_close(&h);

	return true;
}
