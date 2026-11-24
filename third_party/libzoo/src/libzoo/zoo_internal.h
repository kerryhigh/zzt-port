/**
 * Copyright (c) 2020 Adrian Siekierka
 *
 * Based on a reconstruction of code from ZZT,
 * Copyright 1991 Epic MegaGames, used with permission.
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

// zoo_internal.h - private libzoo includes

#ifndef __ZOO_INTERNAL_H__
#define __ZOO_INTERNAL_H__

#include "zoo.h"

// platform/compiler-specific hacks

#if __STDC_VERSION__ >= 199901L
#define ZOO_INLINE inline
#else
#define ZOO_INLINE
#endif

#if defined(__GNUC__) && defined(ZOO_OPTIMIZE)
#define ZOO_FILE_OPTIMIZE_SIZE _Pragma("GCC optimize (\"Os\")")
#else
#define ZOO_FILE_OPTIMIZE_SIZE
#endif

#ifdef ZOO_PLATFORM_GBA
#define GBA_FAST_CODE __attribute__((section(".iwram"), long_call, target("arm")))
#else
#define GBA_FAST_CODE
#endif

#ifdef ZOO_USE_ROM_POINTERS
// Global function.
bool platform_is_rom_ptr(void *ptr);
#else
// No ROM pointers.
#define platform_is_rom_ptr(ptr) 0
#endif

// zoo_element.c

extern const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1];

// zoo_game.c

extern const char zoo_line_chars[16];
extern const char zoo_color_names[8][8];
extern const zoo_stat zoo_stat_template_default;
char *zoo_board_data_heap_alloc(zoo_board *board, uint32_t size, uint32_t *offset);
void zoo_board_data_heap_release(zoo_board *board, char *data, uint32_t reuse_order);
void zoo_board_data_heap_mark_allocated(zoo_board *board, uint32_t offset, uint32_t size);
void zoo_board_data_heap_reset(zoo_board *board);
void zoo_io_world_info_clear_pascal_strings(zoo_world_info *info);
void zoo_io_pstring_assign_pascal(char *text, size_t max_length, uint8_t *raw_data,
	uint8_t *raw_length, bool *raw_valid, const char *value, size_t value_length);
bool zoo_io_pstring_matches_pascal(const char *text, size_t max_length, const uint8_t *raw_data,
		uint8_t raw_length, bool raw_valid, const char *value, size_t value_length);

extern const int16_t zoo_diagonal_delta_x[8];
extern const int16_t zoo_diagonal_delta_y[8];
extern const int16_t zoo_neighbor_delta_x[4];
extern const int16_t zoo_neighbor_delta_y[4];

// zoo_oop_label_cache.c

void zoo_oop_label_cache_build(zoo_state *state, int16_t stat_id);
int16_t zoo_oop_label_cache_search(zoo_state *state, int16_t stat_id, const char *object_message, bool zapped);
void zoo_oop_label_cache_zap(zoo_state *state, int16_t stat_id, int16_t label_data_pos, bool zapped, bool recurse, const char *label);

// zoo_window.c

void zoo_window_sort(zoo_state *state, zoo_text_window *window);

// zoo_window_classic.c

typedef enum {
    ZOO_WINDOW_PATTERN_TOP,
    ZOO_WINDOW_PATTERN_BOTTOM,
    ZOO_WINDOW_PATTERN_INNER,
    ZOO_WINDOW_PATTERN_SEPARATOR
} zoo_window_pattern_type;

void zoo_window_draw_pattern(zoo_state *state, int16_t x, int16_t y, int16_t width, uint8_t color, zoo_window_pattern_type ptype);

#endif /* __ZOO_H__ */
