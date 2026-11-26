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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "zoo_internal.h"

const char zoo_line_chars[16] = "\xF9\xD0\xD2\xBA\xB5\xBC\xBB\xB9\xC6\xC8\xC9\xCC\xCD\xCA\xCB\xCE";
// FIX: preserve 1-indexing and fix messages for black keys/doors
const char zoo_color_names[8][8] = {
	"Black", "Blue", "Green", "Cyan", "Red", "Purple", "Yellow", "White"
};
const zoo_stat zoo_stat_template_default = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	-1, -1,
	{ 0, 0 },
	NULL, 0, 0
};

const int16_t zoo_diagonal_delta_x[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
const int16_t zoo_diagonal_delta_y[8] = {1, 1, 1, 0, -1, -1, -1, 0};
const int16_t zoo_neighbor_delta_x[4] = {0, 0, -1, 1};
const int16_t zoo_neighbor_delta_y[4] = {-1, 1, 0, 0};

static const zoo_tile zoo_board_tile_edge = {ZOO_E_BOARD_EDGE, 0x00};
static const zoo_tile zoo_board_tile_border = {ZOO_E_NORMAL, 0x0E};

static void *zoo_board_data_heap_move(void *dest, const void *src, size_t size) {
	uint8_t *d = (uint8_t *) dest;
	const uint8_t *s = (const uint8_t *) src;
	if ((uintptr_t) d < (uintptr_t) s) {
		for (size_t i = 0; i < size; i++) d[i] = s[i];
	} else if ((uintptr_t) d > (uintptr_t) s) {
		for (size_t i = size; i > 0; i--) d[i - 1] = s[i - 1];
	}
	return dest;
}

static bool zoo_board_data_heap_reserve(zoo_board *board, uint32_t size) {
	uint32_t required;
	uint32_t capacity;
	char *heap;
	int i;

	if (board->data_heap_used > UINT32_MAX - size) return false;
	required = board->data_heap_used + size;
	if (required <= board->data_heap_capacity) return true;

	capacity = board->data_heap_capacity > 0 ? board->data_heap_capacity : 256;
	while (capacity < required) {
		if (capacity > UINT32_MAX / 2) {
			capacity = required;
			break;
		}
		capacity *= 2;
	}

	heap = (char *) malloc(capacity);
	if (heap == NULL) return false;
	if (board->data_heap != NULL) {
		memcpy(heap, board->data_heap, board->data_heap_used);
		free(board->data_heap);
	}
	board->data_heap = heap;
	board->data_heap_capacity = capacity;
	for (i = 0; i <= ZOO_MAX_STAT + 1; i++) {
		if (board->stats[i].data_in_heap) {
			board->stats[i].data = heap + board->stats[i].data_heap_offset;
		}
	}
	return true;
}

static bool zoo_board_data_heap_insert_segment(zoo_board *board, uint16_t index,
		const zoo_data_heap_segment *segment) {
	if (board->data_heap_segment_count >= ZOO_MAX_DATA_HEAP_SEGMENTS) return false;
	zoo_board_data_heap_move(&board->data_heap_segments[index + 1], &board->data_heap_segments[index],
		(sizeof(zoo_data_heap_segment) * (board->data_heap_segment_count - index)));
	board->data_heap_segments[index] = *segment;
	board->data_heap_segment_count++;
	return true;
}

char *zoo_board_data_heap_alloc(zoo_board *board, uint32_t size, uint32_t *offset) {
	uint16_t i;
	int oldest_index = -1;
	uint32_t oldest_order = UINT32_MAX;

	if (size == 0 || board == NULL || offset == NULL) return NULL;
	for (i = 0; i < board->data_heap_segment_count; i++) {
		zoo_data_heap_segment *segment = &board->data_heap_segments[i];
		if (segment->is_free && segment->size >= size
			&& segment->reuse_order < oldest_order) {
			oldest_order = segment->reuse_order;
			oldest_index = i;
		}
	}

	if (oldest_index >= 0) {
		zoo_data_heap_segment *segment = &board->data_heap_segments[oldest_index];
		uint32_t old_size = segment->size;
		uint32_t free_order = segment->reuse_order;
		uint32_t split_boundary_size = segment->split_boundary_size;
		bool free_pending = segment->reuse_pending;
		*offset = segment->offset;
		segment->size = size;
		segment->is_free = false;
		segment->reuse_order = 0;
		segment->reuse_pending = false;
		segment->split_boundary_size = 0;
		if (old_size > size) {
			zoo_data_heap_segment tail;
			tail.offset = *offset + size;
			tail.size = old_size - size;
			tail.reuse_order = free_order;
			tail.split_boundary_size = split_boundary_size > size
				? split_boundary_size - size : 0;
			tail.begins_after_original_block = false;
			tail.is_free = true;
			tail.reuse_pending = free_pending;
			if (!zoo_board_data_heap_insert_segment(board, (uint16_t) (oldest_index + 1), &tail)) {
				segment->size = old_size;
				segment->is_free = true;
				segment->reuse_order = free_order;
				segment->reuse_pending = free_pending;
				segment->split_boundary_size = split_boundary_size;
				return NULL;
			}
		}
		return board->data_heap + *offset;
	}

	if (board->data_heap_segment_count >= ZOO_MAX_DATA_HEAP_SEGMENTS
		|| !zoo_board_data_heap_reserve(board, size)) return NULL;
	*offset = board->data_heap_used;
	board->data_heap_used += size;
	zoo_data_heap_segment segment;
	segment.offset = *offset;
	segment.size = size;
	segment.reuse_order = 0;
	segment.split_boundary_size = 0;
	segment.begins_after_original_block = *offset > 0;
	segment.is_free = false;
	segment.reuse_pending = false;
	if (!zoo_board_data_heap_insert_segment(board, board->data_heap_segment_count, &segment)) {
		board->data_heap_used = *offset;
		return NULL;
	}
	return board->data_heap + *offset;
}

void zoo_board_data_heap_release(zoo_board *board, char *data, uint32_t reuse_order) {
	uintptr_t base, address;
	uint32_t offset;
	uint16_t i;

	if (board == NULL || board->data_heap == NULL || data == NULL) return;
	base = (uintptr_t) board->data_heap;
	address = (uintptr_t) data;
	if (address < base || address - base >= board->data_heap_used) return;
	offset = (uint32_t) (address - base);
	for (i = 0; i < board->data_heap_segment_count; i++) {
		zoo_data_heap_segment *segment = &board->data_heap_segments[i];
		if (segment->offset != offset || segment->is_free) continue;
		if (reuse_order == 0) {
			reuse_order = ++board->data_heap_next_reuse_order;
		} else if (reuse_order > board->data_heap_next_reuse_order) {
			board->data_heap_next_reuse_order = reuse_order;
		}
		segment->is_free = true;
		segment->reuse_order = reuse_order;
		segment->reuse_pending = reuse_order != 0;
		break;
	}
	if (i >= board->data_heap_segment_count) return;

	i = 0;
	while (i + 1 < board->data_heap_segment_count) {
		zoo_data_heap_segment *left = &board->data_heap_segments[i];
		zoo_data_heap_segment *right = &board->data_heap_segments[i + 1];
		if (left->is_free && right->is_free
			&& left->offset + left->size == right->offset) {
			if (left->split_boundary_size == 0 && right->begins_after_original_block) {
				left->split_boundary_size = left->size;
			}
			if (right->reuse_order < left->reuse_order) left->reuse_order = right->reuse_order;
			left->size += right->size;
			left->reuse_pending = left->reuse_pending || right->reuse_pending;
			zoo_board_data_heap_move(right, right + 1,
				(sizeof(zoo_data_heap_segment) * (board->data_heap_segment_count - i - 2)));
			board->data_heap_segment_count--;
		} else {
			i++;
		}
	}
}

void zoo_board_data_heap_mark_allocated(zoo_board *board, uint32_t offset, uint32_t size) {
	int16_t i;
	if (board == NULL || size == 0 || offset > UINT32_MAX - size) return;
	for (i = 1; i <= board->stat_count; i++) {
		zoo_stat *candidate = &board->stats[i];
		if (candidate->data_in_heap && candidate->data_reuse_pending
			&& candidate->data_heap_offset >= offset
			&& candidate->data_heap_offset < offset + size) {
			candidate->data_reuse_pending = false;
			candidate->data_reuse_order = 0;
		}
	}
}

void zoo_board_data_heap_reset(zoo_board *board) {
	int i;
	if (board == NULL) return;
	free(board->data_heap);
	board->data_heap = NULL;
	board->data_heap_capacity = 0;
	board->data_heap_used = 0;
	board->data_heap_next_reuse_order = 0;
	board->data_heap_segment_count = 0;
	memset(board->data_heap_segments, 0, sizeof(board->data_heap_segments));
	for (i = 0; i <= ZOO_MAX_STAT + 1; i++) {
		if (board->stats[i].data_in_heap) {
			board->stats[i].data = NULL;
			board->stats[i].data_len = 0;
			board->stats[i].data_in_heap = false;
			board->stats[i].data_heap_offset = 0;
		}
	}
}

void zoo_board_change(zoo_state *state, int16_t board_id) {
	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element = ZOO_E_PLAYER;
	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].color
		= zoo_element_defs[ZOO_E_PLAYER].color;
	
	state->error_value = zoo_board_close(state);
	if (state->error_value) return;

	state->error_value = zoo_board_open(state, board_id);
	if (state->error_value) return;
}

void zoo_board_create(zoo_state *state) {
	int16_t i, ix, iy;
	zoo_board_data_heap_reset(&state->board);

	// Pascal short-string assignment changes the prefix and length, not the fixed payload tail.
	zoo_io_pstring_assign_pascal(state->board.name, sizeof(state->board.name) - 1,
		state->board.io_name_data, &state->board.io_name_length,
		&state->board.io_name_valid, "", 0);
	zoo_io_pstring_assign_pascal(state->board.info.message, ZOO_LEN_MESSAGE,
		state->board.info.io_message_data, &state->board.info.io_message_length,
		&state->board.info.io_message_valid, "", 0);
	state->board.info.max_shots = 255;
	state->board.info.is_dark = false;
	state->board.info.reenter_when_zapped = false;
	state->board.info.io_is_dark_byte = 0;
	state->board.info.io_reenter_when_zapped_byte = 0;
	state->board.info.io_boolean_bytes_valid = true;
	state->board.info.time_limit_sec = 0;
	for (i = 0; i < 4; i++)
		state->board.info.neighbor_boards[i] = 0;

	for (ix = 0; ix <= ZOO_BOARD_WIDTH + 1; ix++) {
		state->board.tiles[ix][0] = zoo_board_tile_edge;
		state->board.tiles[ix][ZOO_BOARD_HEIGHT + 1] = zoo_board_tile_edge;
	}

	for (iy = 0; iy <= ZOO_BOARD_HEIGHT + 1; iy++) {
		state->board.tiles[0][iy] = zoo_board_tile_edge;
		state->board.tiles[ZOO_BOARD_WIDTH + 1][iy] = zoo_board_tile_edge;
	}

	for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
		for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
			state->board.tiles[ix][iy].element = ZOO_E_EMPTY;
			state->board.tiles[ix][iy].color = 0x00;
		}
	}

	for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
		state->board.tiles[ix][1] = zoo_board_tile_border;
		state->board.tiles[ix][ZOO_BOARD_HEIGHT] = zoo_board_tile_border;
	}

	for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
		state->board.tiles[1][iy] = zoo_board_tile_border;
		state->board.tiles[ZOO_BOARD_WIDTH][iy] = zoo_board_tile_border;
	}

	state->board.tiles[ZOO_BOARD_WIDTH / 2][ZOO_BOARD_HEIGHT / 2].element = ZOO_E_PLAYER;
	state->board.tiles[ZOO_BOARD_WIDTH / 2][ZOO_BOARD_HEIGHT / 2].color
		= zoo_element_defs[ZOO_E_PLAYER].color;
	state->board.stat_count = 0;
	state->board.stats[0].x = ZOO_BOARD_WIDTH / 2;
	state->board.stats[0].y = ZOO_BOARD_HEIGHT / 2;
	state->board.stats[0].cycle = 1;
	state->board.stats[0].under.element = ZOO_E_EMPTY;
	state->board.stats[0].under.color = 0x00;
	state->board.stats[0].data = NULL;
	state->board.stats[0].data_len = 0;
}

void zoo_reset_message_flags(zoo_state *state) {
	memset(&(state->msg_flags), 1, sizeof(zoo_state_message));
}

void zoo_world_create(zoo_state *state) {
	int16_t i;

	state->world.board_count = 0;
	state->world.board_len[0] = 0;
	zoo_reset_message_flags(state);

	zoo_board_create(state);
	state->world.info.is_save = false;
	state->world.info.io_is_save_byte = 0;
	state->world.info.io_boolean_bytes_valid = true;
	state->world.info.current_board = 0;
	state->world.info.ammo = 0;
	state->world.info.gems = 0;
	state->world.info.health = 100;
	state->world.info.energizer_ticks = 0;
	state->world.info.torches = 0;
	state->world.info.torch_ticks = 0;
	state->world.info.score = 0;
	state->world.info.board_time_sec = 0;
	state->world.info.board_time_hsec = 0;
	for (i = 0; i < 7; i++) {
		state->world.info.keys[i] = false;
		state->world.info.io_key_bytes[i] = 0;
	}
	zoo_board_change(state, 0);
	zoo_io_pstring_assign_pascal(state->board.name, sizeof(state->board.name) - 1,
		state->board.io_name_data, &state->board.io_name_length,
		&state->board.io_name_valid, "Title screen", sizeof("Title screen") - 1);
	// The Android host owns the active file name in MainActivity.activeFilename;
	// editor entry/reset callers choose the corresponding app file target.
	zoo_io_world_info_clear_pascal_strings(&state->world.info);
}

GBA_FAST_CODE
void zoo_board_draw_tile(zoo_state *state, int16_t x, int16_t y) {
	uint8_t ch;
	zoo_tile *tile;

	if (state->d_video->func_write == NULL) {
		return;
	}

	// FIX: bounds check
	if (x >= 1 && y >= 1 && x <= ZOO_BOARD_WIDTH && y <= ZOO_BOARD_HEIGHT) {
		tile = &state->board.tiles[x][y];

		// darkness handling
		if (
			state->board.info.is_dark
			&& !zoo_element_defs[tile->element].visible_in_dark
			&& (
				(state->world.info.torch_ticks <= 0)
				|| (zoo_dist_sq2(state->board.stats[0].x - x, state->board.stats[0].y - y) >= ZOO_TORCH_DSQ)
			) && !state->force_darkness_off
		) {
			state->d_video->func_write(state->d_video, x - 1, y - 1, 0x07, '\xB0');
			return;
		}

		if (tile->element == ZOO_E_EMPTY) {
			state->d_video->func_write(state->d_video, x - 1, y - 1, 0x0F, ' ');
		} else if (zoo_element_defs[tile->element].has_draw_func) {
			zoo_element_defs[tile->element].draw_func(state, x, y, &ch);
			state->d_video->func_write(state->d_video, x - 1, y - 1, tile->color, ch);
		} else if (tile->element < ZOO_E_TEXT_MIN) {
			state->d_video->func_write(state->d_video, x - 1, y - 1, tile->color, zoo_element_defs[tile->element].character);
		} else {
			// text drawing
			if (tile->element == ZOO_E_TEXT_WHITE) {
				state->d_video->func_write(state->d_video, x - 1, y - 1, 0x0F, tile->color);
			} else {
				uint8_t text_color = (uint8_t) (((tile->element - ZOO_E_TEXT_MIN + 1) << 4)
					| (state->d_video->monochrome ? 0 : 0x0F));
				state->d_video->func_write(state->d_video, x - 1, y - 1,
					text_color, tile->color);
			}
		}
	}
}

GBA_FAST_CODE
void zoo_board_draw_border(zoo_state *state) {
	int i;

	for (i = 1; i <= ZOO_BOARD_WIDTH; i++) {
		zoo_board_draw_tile(state, i, 1);
		zoo_board_draw_tile(state, i, ZOO_BOARD_HEIGHT);
	}

	for (i = 1; i <= ZOO_BOARD_HEIGHT; i++) {
		zoo_board_draw_tile(state, 1, i);
		zoo_board_draw_tile(state, ZOO_BOARD_WIDTH, i);
	}
}

GBA_FAST_CODE
void zoo_board_draw(zoo_state *state) {
	int ix, iy;

	for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
		for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
			zoo_board_draw_tile(state, ix, iy);
		}
	}
}

void zoo_board_draw_transition(zoo_state *state) {
	int16_t i;
	int16_t x, y;
	const uint16_t count = ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT;

	if (state->d_video->func_transition_start != NULL) {
		state->d_video->func_transition_start(state->d_video);
	}

	for (i = 0; i < count; i++) {
		uint16_t cell = state->transition_order[i];
		x = (cell % ZOO_BOARD_WIDTH) + 1;
		y = (cell / ZOO_BOARD_WIDTH) + 1;
		state->d_video->func_write(state->d_video, x - 1, y - 1, 0x05, '\xDB');
	}

	zoo_board_draw_border(state);
	for (i = 0; i < count; i++) {
		uint16_t cell = state->transition_order[i];
		x = (cell % ZOO_BOARD_WIDTH) + 1;
		y = (cell / ZOO_BOARD_WIDTH) + 1;
		zoo_board_draw_tile(state, x, y);
	}

	if (state->d_video->func_transition_finish != NULL) {
		state->d_video->func_transition_finish(state->d_video,
			state->transition_order, count, ZOO_VIDEO_TRANSITION_BOARD_CHANGE);
	}
}

void zoo_board_draw_fill_transition(zoo_state *state, uint8_t color) {
	int16_t i;
	int16_t x, y;
	const uint16_t count = ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT;

	if (state->d_video->func_transition_start != NULL) {
		state->d_video->func_transition_start(state->d_video);
	}

	for (i = 0; i < count; i++) {
		uint16_t cell = state->transition_order[i];
		x = (cell % ZOO_BOARD_WIDTH) + 1;
		y = (cell / ZOO_BOARD_WIDTH) + 1;
		state->d_video->func_write(state->d_video, x - 1, y - 1, color, '\xDB');
	}

	if (state->d_video->func_transition_finish != NULL) {
		state->d_video->func_transition_finish(state->d_video,
			state->transition_order, count, ZOO_VIDEO_TRANSITION_FILL);
	}
}

void zoo_board_draw_ordered(zoo_state *state) {
	int16_t i;
	int16_t x, y;
	const uint16_t count = ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT;

	if (state->d_video->func_transition_start != NULL) {
		state->d_video->func_transition_start(state->d_video);
	}

	zoo_board_draw_border(state);
	for (i = 0; i < count; i++) {
		uint16_t cell = state->transition_order[i];
		x = (cell % ZOO_BOARD_WIDTH) + 1;
		y = (cell / ZOO_BOARD_WIDTH) + 1;
		zoo_board_draw_tile(state, x, y);
	}

	if (state->d_video->func_transition_finish != NULL) {
		state->d_video->func_transition_finish(state->d_video,
			state->transition_order, count, ZOO_VIDEO_TRANSITION_REDRAW);
	}
}

void zoo_stat_add(zoo_state *state, int16_t tx, int16_t ty, uint8_t element, int16_t color, int16_t tcycle, const zoo_stat *stat_template) {
	zoo_stat *stat;

	if (state->board.stat_count < ZOO_MAX_STAT) {
		state->board.stat_count++;
		stat = &(state->board.stats[state->board.stat_count]);

		memcpy(stat, stat_template, sizeof(zoo_stat));
		stat->x = tx;
		stat->y = ty;
		stat->cycle = tcycle;
		stat->under = state->board.tiles[tx][ty];
		stat->data_pos = 0;
		stat->data_reuse_pending = false;
		stat->data_reuse_order = 0;
		stat->data_in_heap = false;
		stat->data_heap_offset = 0;

		if (stat_template->data != NULL) {
			char *source_copy = NULL;
			uint32_t offset = 0;
			if (stat->data_len > 0) {
				source_copy = (char *) malloc((size_t) stat->data_len);
				if (source_copy != NULL) {
					memcpy(source_copy, stat_template->data, (size_t) stat->data_len);
				}
			}

			stat->data = stat->data_len > 0
				? zoo_board_data_heap_alloc(&state->board, (uint32_t) stat->data_len, &offset)
				: NULL;
			if (stat->data != NULL && source_copy != NULL) {
				stat->data_in_heap = true;
				stat->data_heap_offset = offset;
				memcpy(stat->data, source_copy, (size_t) stat->data_len);
				zoo_board_data_heap_mark_allocated(&state->board, offset,
					(uint32_t) stat->data_len);
			} else {
				if (stat->data != NULL) {
					zoo_board_data_heap_release(&state->board, stat->data, 0);
				}
				stat->data = malloc((size_t) stat->data_len);
				if (stat->data != NULL && source_copy != NULL) {
					memcpy(stat->data, source_copy, (size_t) stat->data_len);
				}
			}
			free(source_copy);
		}

		if (zoo_element_defs[state->board.tiles[tx][ty].element].placeable_on_top) {
			state->board.tiles[tx][ty].color = (state->board.tiles[tx][ty].color & 0x70) | (color & 0x0F);
		} else {
			state->board.tiles[tx][ty].color = color;
		}
		state->board.tiles[tx][ty].element = element;

		if (ty > 0) {
			zoo_board_draw_tile(state, tx, ty);
		}
	}
}

void zoo_stat_remove(zoo_state *state, int16_t stat_id) {
	int i;
	zoo_stat *stat;

	// FIX: bounds check
	if (stat_id < 0 || stat_id > state->board.stat_count) return;
	stat = &(state->board.stats[stat_id]);

	if (stat->data_len != 0) {
		for (i = 1; i <= state->board.stat_count; i++) {
			if (i == stat_id) continue;
			if (state->board.stats[i].data == stat->data) goto StatDataInUse;
		}
	}
	if (stat->data_in_heap) {
		zoo_board_data_heap_release(&state->board, stat->data, 0);
	} else {
		zoo_stat_free(stat);
	}

StatDataInUse:
	if (stat_id < state->current_stat_tick) {
		state->current_stat_tick--;
	}

	state->board.tiles[stat->x][stat->y] = stat->under;
	if (stat->y > 0) {
		zoo_board_draw_tile(state, stat->x, stat->y);
	}

	for (i = 1; i <= state->board.stat_count; i++) {
		if (state->board.stats[i].follower >= stat_id) {
			if (state->board.stats[i].follower == stat_id) {
				state->board.stats[i].follower = -1;
			} else {
				state->board.stats[i].follower--;
			}
		}

		if (state->board.stats[i].leader >= stat_id) {
			if (state->board.stats[i].leader == stat_id) {
				state->board.stats[i].leader = -1;
			} else {
				state->board.stats[i].leader--;
			}
		}
	}

	for (i = stat_id + 1; i <= state->board.stat_count; i++) {
		state->board.stats[i - 1] = state->board.stats[i];
	}
	state->board.stat_count--;
}

GBA_FAST_CODE
int16_t zoo_stat_get_id(zoo_state *state, int16_t x, int16_t y) {
	int16_t i;
	zoo_stat *stat = state->board.stats;

	for (i = 0; i <= state->board.stat_count; i++, stat++) {
		if (x == stat->x && y == stat->y)
			return i;
	}

	return -1;
}

GBA_FAST_CODE
zoo_stat *zoo_stat_get(zoo_state *state, int16_t x, int16_t y, int16_t *stat_id) {
	int16_t i;
	zoo_stat *stat = state->board.stats;

	for (i = 0; i <= state->board.stat_count; i++, stat++) {
		if (x == stat->x && y == stat->y) {
			if (stat_id != NULL) {
				*stat_id = i;
			}
			return stat;
		}
	}

	if (stat_id != NULL) {
		*stat_id = -1;
	}
	return NULL;
}

void zoo_stat_move(zoo_state *state, int16_t stat_id, int16_t new_x, int16_t new_y) {
	zoo_stat *stat;
	zoo_tile under;
	int16_t old_x, old_y;
	int16_t ix, iy;

	// FIX: bounds check
	if (stat_id < 0 || stat_id > state->board.stat_count) return;

	stat = &(state->board.stats[stat_id]);

	under = stat->under;
	stat->under = state->board.tiles[new_x][new_y];

	if (state->board.tiles[stat->x][stat->y].element == ZOO_E_PLAYER) {
		state->board.tiles[new_x][new_y].color = state->board.tiles[stat->x][stat->y].color;
	} else if (state->board.tiles[new_x][new_y].element == ZOO_E_EMPTY) {
		state->board.tiles[new_x][new_y].color = state->board.tiles[stat->x][stat->y].color & 0x0F;
	} else {
		state->board.tiles[new_x][new_y].color = (state->board.tiles[stat->x][stat->y].color & 0x0F)
			+ (state->board.tiles[new_x][new_y].color & 0x70);
	}

	state->board.tiles[new_x][new_y].element = state->board.tiles[stat->x][stat->y].element;
	state->board.tiles[stat->x][stat->y] = under;

	old_x = stat->x;
	old_y = stat->y;
	stat->x = new_x;
	stat->y = new_y;

	zoo_board_draw_tile(state, stat->x, stat->y);
	zoo_board_draw_tile(state, old_x, old_y);

	if (stat_id == 0 && state->board.info.is_dark && state->world.info.torch_ticks > 0) {
		if (zoo_dist_sq(old_x - stat->x, old_y - stat->y) == 1) {
			for (ix = stat->x - ZOO_TORCH_DX - 3; ix <= stat->x + ZOO_TORCH_DX + 3; ix++)
			if (ix >= 1 && ix <= ZOO_BOARD_WIDTH)
			for (iy = stat->y - ZOO_TORCH_DY - 3; iy <= stat->y + ZOO_TORCH_DY + 3; iy++)
			if (iy >= 1 && iy <= ZOO_BOARD_HEIGHT) {
				if ((zoo_dist_sq2(ix - old_x, iy - old_y) < ZOO_TORCH_DSQ)
					^ (zoo_dist_sq2(ix - new_x, iy - new_y) < ZOO_TORCH_DSQ)) {
					zoo_board_draw_tile(state, ix, iy);
				}
			}
		} else {
			zoo_draw_player_surroundings(state, old_x, old_y, 0);
			zoo_draw_player_surroundings(state, stat->x, stat->y, 0);
		}
	}
}

void zoo_display_message(zoo_state *state, int16_t duration, char *message) {
	int i;

	i = zoo_stat_get_id(state, 0, 0);
	if (i != -1) {
		zoo_stat_remove(state, i);
		zoo_board_draw_border(state);
	}

	if (message[0] != '\0') {
		zoo_stat_add(state, 0, 0, ZOO_E_MESSAGE_TIMER, 0, 1, &zoo_stat_template_default);
		state->board.stats[state->board.stat_count].p2 = duration / (state->tick_duration + 1);
		zoo_io_pstring_assign_pascal(state->board.info.message, ZOO_LEN_MESSAGE,
			state->board.info.io_message_data, &state->board.info.io_message_length,
			&state->board.info.io_message_valid, message,
			strnlen(message, ZOO_LEN_MESSAGE));
	}
}

void zoo_board_damage_stat(zoo_state *state, int16_t stat_id) {
	int16_t old_x, old_y;
	zoo_stat *stat;

	// FIX: bounds check
	if (stat_id < 0 || stat_id > state->board.stat_count) return;

	stat = &(state->board.stats[stat_id]);
	if (stat_id == 0) {
		if (state->world.info.health > 0) {
			state->world.info.health -= 10;
			if (state->world.info.health < 0) {
				state->world.info.health = 0;
			}

			state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_HEALTH);
			zoo_display_message(state, 100, "Ouch!");

			state->board.tiles[stat->x][stat->y].color = 0x70 | (zoo_element_defs[ZOO_E_PLAYER].color & 0x0F);

			if (state->world.info.health > 0) {
				state->world.info.board_time_sec = 0;
				if (state->board.info.reenter_when_zapped) {
					zoo_sound_queue_const(&(state->sound), 4, "\x20\x01\x23\x01\x27\x01\x30\x01\x10\x01");

					// move player to start
					state->board.tiles[stat->x][stat->y].element = ZOO_E_EMPTY;
					zoo_board_draw_tile(state, stat->x, stat->y);
					old_x = stat->x;
					old_y = stat->y;
					stat->x = state->board.info.start_player_x;
					stat->y = state->board.info.start_player_y;
					zoo_draw_player_surroundings(state, old_x, old_y, 0);
					zoo_draw_player_surroundings(state, stat->x, stat->y, 0);

					state->game_paused = true;
				}
				zoo_sound_queue_const(&(state->sound), 4, "\x10\x01\x20\x01\x13\x01\x23\x01");
			} else {
				zoo_sound_queue_const(&(state->sound), 5,
					"\x20\x03\x23\x03\x27\x03\x30\x03"
					"\x27\x03\x2A\x03\x32\x03\x37\x03"
					"\x35\x03\x38\x03\x40\x03\x45\x03\x10\x0A"
				);
			}
		}
	} else {
		switch (state->board.tiles[stat->x][stat->y].element) {
			case ZOO_E_BULLET: zoo_sound_queue_const(&(state->sound), 3, "\x20\x01"); break;
			case ZOO_E_OBJECT: break;
			default: zoo_sound_queue_const(&(state->sound), 3, "\x40\x01\x10\x01\x50\x01\x30\x01"); break;
		}
		zoo_stat_remove(state, stat_id);
	}
}

void zoo_board_damage_tile(zoo_state *state, int16_t x, int16_t y) {
	int16_t stat_id = zoo_stat_get_id(state, x, y);

	if (stat_id != -1) {
		zoo_board_damage_stat(state, stat_id);
	} else {
		state->board.tiles[x][y].element = ZOO_E_EMPTY;
		zoo_board_draw_tile(state, x, y);
	}
}

void zoo_board_attack_tile(zoo_state *state, int16_t attacker_stat_id, int16_t x, int16_t y) {
	if ((attacker_stat_id == 0) && (state->world.info.energizer_ticks > 0)) {
		state->world.info.score += zoo_element_defs[state->board.tiles[x][y].element].score_value;
		state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_SCORE);
	} else {
		zoo_board_damage_stat(state, attacker_stat_id);
	}

	if ((attacker_stat_id > 0) && (attacker_stat_id <= state->current_stat_tick)) {
		state->current_stat_tick--;
	}

	if ((state->board.tiles[x][y].element == ZOO_E_PLAYER) && (state->world.info.energizer_ticks > 0)) {
		state->world.info.score += zoo_element_defs[state->board.tiles
			[state->board.stats[attacker_stat_id].x]
			[state->board.stats[attacker_stat_id].y]
		.element].score_value;
		state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_SCORE);
	} else {
		zoo_board_damage_tile(state, x, y);
		zoo_sound_queue_const(&(state->sound), 2, "\x10\x01");
	}
}

bool zoo_board_shoot(zoo_state *state, uint8_t element, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t param1) {
	if ((zoo_element_defs[state->board.tiles[x+dx][y+dy].element].walkable)
		|| (state->board.tiles[x+dx][y+dy].element == ZOO_E_WATER))
	{
		zoo_stat_add(state, x + dx, y + dy, element, zoo_element_defs[element].color, 1, &zoo_stat_template_default);
		state->board.stats[state->board.stat_count].p1 = param1;
		state->board.stats[state->board.stat_count].step_x = dx;
		state->board.stats[state->board.stat_count].step_y = dy;
		state->board.stats[state->board.stat_count].p2 = 100;
		return true;
	} else if (state->board.tiles[x+dx][y+dy].element == ZOO_E_BREAKABLE
		|| (
			zoo_element_defs[state->board.tiles[x+dx][y+dy].element].destructible
			&& ((state->board.tiles[x+dx][y+dy].element == ZOO_E_PLAYER) == (param1 != 0))
			&& (state->world.info.energizer_ticks <= 0)
		)
	) {
		zoo_board_damage_tile(state, x + dx, y + dy);
		zoo_sound_queue_const(&(state->sound), 2, "\x10\x01");
		return true;
	} else {
		return false;
	}
}

void zoo_calc_direction_rnd(zoo_state *state, int16_t *dx, int16_t *dy) {
	*dx = state->func_random(state, 3) - 1;
	*dy = (*dx == 0) ? (state->func_random(state, 2) * 2 - 1) : 0;
}

void zoo_calc_direction_seek(zoo_state *state, int16_t x, int16_t y, int16_t *dx, int16_t *dy) {
	*dx = 0;
	*dy = 0;

	if ((state->func_random(state, 2) < 1) || (state->board.stats[0].y == y)) {
		*dx = zoo_signum(state->board.stats[0].x - x);
	}

	if (*dx == 0) {
		*dy = zoo_signum(state->board.stats[0].y - y);
	}

	if (state->world.info.energizer_ticks > 0) {
		*dx = -(*dx);
		*dy = -(*dy);
	}
}

void zoo_board_enter(zoo_state *state) {
	state->board.info.start_player_x = state->board.stats[0].x;
	state->board.info.start_player_y = state->board.stats[0].y;

	if (state->board.info.is_dark) {
		if (state->msg_flags.hint_torch) {
			zoo_display_message(state, 200, "Room is dark - you need to light a torch!");
			state->msg_flags.hint_torch = false;
		}
	}

	state->world.info.board_time_sec = 0;
	state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_ALL);
}

void zoo_board_passage_teleport(zoo_state *state, int16_t x, int16_t y) {
	zoo_stat *stat;
	int16_t old_board;
	uint8_t color;
	int16_t ix, iy, new_x, new_y;

	stat = zoo_stat_get(state, x, y, NULL);
	color = state->board.tiles[x][y].color;
	old_board = state->world.info.current_board;
	zoo_board_change(state, stat != NULL ? stat->p3 : 0);

	new_x = 0;
	for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
		for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
			if (
				state->board.tiles[ix][iy].element == ZOO_E_PASSAGE
				&& state->board.tiles[ix][iy].color == color
			) {
				new_x = ix;
				new_y = iy;
			}
		}
	}

	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element = ZOO_E_EMPTY;
	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].color = 0x00;
	if (new_x != 0) {
		state->board.stats[0].x = new_x;
		state->board.stats[0].y = new_y;
	}

	state->game_paused = true;
	zoo_sound_queue_const(&(state->sound), 4,
		"\x30\x01\x34\x01\x37\x01"
		"\x31\x01\x35\x01\x38\x01"
		"\x32\x01\x36\x01\x39\x01"
		"\x33\x01\x37\x01\x3A\x01"
		"\x34\x01\x38\x01\x40\x01");
	zoo_board_draw_transition(state);
	zoo_board_enter(state);
}

void zoo_game_debug_command(zoo_state *state, const char *in_cmd) {
	int i, ix, iy;
	bool toggle = true;
	char cmd[51];

	for (i = 0; i <= strlen(in_cmd) && i < sizeof(cmd); i++) {
		cmd[i] = zoo_toupper(in_cmd[i]);
	}
	cmd[50] = '\0';

	if ((cmd[0] == '+') || (cmd[0] == '-')) {
		if (cmd[0] == '-') {
			toggle = false;
		}

		memmove(cmd, cmd + 1, strlen(cmd));

		if (toggle) {
			zoo_flag_set(state, cmd);
		} else {
			zoo_flag_clear(state, cmd);
		}
	}

	state->debug_enabled = zoo_flag_get_id(state, "DEBUG") >= 0;

	if (!strcmp(cmd, "HEALTH")) {
		state->world.info.health += 50;
	} else if (!strcmp(cmd, "AMMO")) {
		state->world.info.ammo += 5;
	} else if (!strcmp(cmd, "KEYS")) {
		for (i = 0; i < 7; i++) {
			state->world.info.keys[i] = true;
			state->world.info.io_key_bytes[i] = 1;
		}
	} else if (!strcmp(cmd, "TORCHES")) {
		state->world.info.torches += 3;
	} else if (!strcmp(cmd, "TIME")) {
		state->world.info.board_time_sec -= 30;
	} else if (!strcmp(cmd, "GEMS")) {
		state->world.info.gems += 5;
	} else if (!strcmp(cmd, "DARK")) {
		state->board.info.is_dark = toggle;
		state->board.info.io_is_dark_byte = toggle ? 1 : 0;
		zoo_board_draw_ordered(state);
	} else if (!strcmp(cmd, "ZAP")) {
		for (i = 0; i < 4; i++) {
			ix = state->board.stats[0].x + zoo_neighbor_delta_x[i];
			iy = state->board.stats[0].y + zoo_neighbor_delta_y[i];
			zoo_board_damage_tile(state, ix, iy);
			state->board.tiles[ix][iy].element = ZOO_E_EMPTY;
			zoo_board_draw_tile(state, ix, iy);
		}
	}

	zoo_sound_queue_const(&(state->sound), 10, "\x27\x04");
	state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_ALL);
}

void zoo_game_start(zoo_state *state, zoo_game_state game_state) {
	state->game_state = game_state;
	state->input.key_pressed = 0;
	state->input.key_buffer_head = 0;
	state->input.key_buffer_count = 0;

	// call stack handling
	while (state->call_stack.call != NULL) {
		zoo_call_pop(&state->call_stack);
	}

	if (game_state == GS_NONE) {
		return;
	}

	// GS_TITLE/GS_PLAY
	state->game_play_exit_requested = false;
	state->current_tick = state->func_random(state, 100);
	state->current_stat_tick = state->board.stat_count + 1;
	state->tick_duration = state->tick_speed * 2;
	/* The Pascal TimerTicks/GetTime clock and TickTimeCounter live across
	   GamePlayLoop calls. zoo_game_start must not restart only the elapsed side
	   of that pair, or the next elapsed check wraps and fires immediately. */

	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element
		= (state->game_state == GS_TITLE) ? ZOO_E_MONITOR : ZOO_E_PLAYER;
	state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].color
		= zoo_element_defs[(state->game_state == GS_TITLE) ? ZOO_E_MONITOR : ZOO_E_PLAYER].color;

	if (state->game_state == GS_TITLE) {
		zoo_display_message(state, 0, "");
		state->game_title_exit_requested = false;
		state->game_paused = false;
	} else if (state->game_state == GS_PLAY) {
		state->game_paused = true;
	}

	zoo_sound_clear_queue(&(state->sound));
	state->sound.block_queueing = false;
}

void zoo_game_stop(zoo_state *state) {
	state->game_state = GS_NONE;
	zoo_sound_clear_queue(&(state->sound));
}

static ZOO_INLINE zoo_tick_retval zoo_call_stack_tick(zoo_state *state) {
	zoo_call call;
	zoo_call *callptr;
	zoo_tick_retval ret;

	// call stack handling
	if (state->call_stack.call != NULL) {
		callptr = state->call_stack.call;
		switch (callptr->type) {
			case CALLBACK:
				// handle these a bit differently for performance
				ret = callptr->args.cb.func(
					state,
					callptr->args.cb.arg
				);
				if (ret == EXIT) {
					if (state->call_stack.call == callptr) {
						zoo_call_pop(&state->call_stack);
					}
					ret = RETURN_IMMEDIATE;
				}
				return ret;
			default:
				// handle these a bit slowly as they are not loops. should be fine
				memcpy(&call, state->call_stack.call, sizeof(zoo_call));
				zoo_call_pop(&state->call_stack);
				state->call_stack.state = call.state;
				state->call_stack.curr_call = &call;
				switch (call.type) {
					case TICK_FUNC:
						call.args.tick.func(state, call.args.tick.stat_id);
						break;
					case TOUCH_FUNC:
						call.args.touch.func(state, call.args.touch.x, call.args.touch.y,
							call.args.touch.source_stat_id, call.args.touch.dx, call.args.touch.dy);
						break;
					default:
						break;
				}
				state->call_stack.curr_call = NULL;
				return RETURN_IMMEDIATE;
		}
	}

	state->call_stack.state = 0;
	return EXIT; // never returned officially, so we repurpose it to mean "continue"
}

static zoo_tick_retval zoo_game_tick(zoo_state *state) {
	int i;
	uint8_t call_state;

	call_state = state->game_tick_state;
	state->game_tick_state = 0;

	// Pascal returns from GamePlayLoop as soon as a tick requests its exit.
	// Do not let remaining board stats continue to run while the outer title
	// loop or the Android menu handles that request.
	if (state->game_play_exit_requested && call_state == 0) {
		return RETURN_NEXT_CYCLE;
	}

	switch (call_state) {
		case 1:
			goto GameTickState1;
		case 2:
			goto GameTickState2;
	}

	if (state->game_paused) {
		if (state->d_video->func_write != NULL) {
			if (zoo_has_hsecs_elapsed(state, &state->tick_counter, 25)) {
				state->game_paused_blink = !state->game_paused_blink;
			}

			if (state->game_paused_blink) {
				state->d_video->func_write(
					state->d_video,
					state->board.stats[0].x - 1,
					state->board.stats[0].y - 1,
					zoo_element_defs[ZOO_E_PLAYER].color,
					zoo_element_defs[ZOO_E_PLAYER].character
				);
			} else {
				if (state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element == ZOO_E_PLAYER) {
					state->d_video->func_write(
						state->d_video,
						state->board.stats[0].x - 1,
						state->board.stats[0].y - 1,
						0x0F, ' '
					);
				} else {
					zoo_board_draw_tile(state, state->board.stats[0].x, state->board.stats[0].y);
				}
			}
		}

		state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_PAUSED);
		zoo_input_update(&state->input);
		// libzoo note: I *think* this should be here.
		// It should fix a scenario where, after being paused,
		// you move twice.
		zoo_input_clear(&state->input);

		if (state->input.delta_x != 0 || state->input.delta_y != 0) {
			// push self
			state->game_tick_state = 1;
			// touch
			zoo_element_defs[state->board.tiles
				[state->board.stats[0].x + state->input.delta_x]
				[state->board.stats[0].y + state->input.delta_y]
			.element].touch_func(state,
				state->board.stats[0].x + state->input.delta_x,
				state->board.stats[0].y + state->input.delta_y,
				0, &state->input.delta_x,
				&state->input.delta_y
			);
			// return
			return RETURN_IMMEDIATE;
		}
		/* Pascal reaches the shared elapsed-time check only after all stats
		   have been ticked. Otherwise its paused repeat yields without moving
		   the stat cursor, which the host represents as the next cycle. */
		if (state->current_stat_tick > state->board.stat_count)
			goto GameTickStateDone;
		return RETURN_NEXT_CYCLE;

GameTickState1:
		if (
			(state->input.delta_x != 0 || state->input.delta_y != 0)
			&& zoo_element_defs[state->board.tiles
				[state->board.stats[0].x + state->input.delta_x]
				[state->board.stats[0].y + state->input.delta_y]
			.element].walkable
		) {
			if (state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element == ZOO_E_PLAYER) {
				zoo_stat_move(state, 0,
					state->board.stats[0].x + state->input.delta_x,
					state->board.stats[0].y + state->input.delta_y
				);
			} else {
				zoo_board_draw_tile(state, state->board.stats[0].x, state->board.stats[0].y);
				state->board.stats[0].x += state->input.delta_x;
				state->board.stats[0].y += state->input.delta_y;
				state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].element = ZOO_E_PLAYER;
				state->board.tiles[state->board.stats[0].x][state->board.stats[0].y].color
					= zoo_element_defs[ZOO_E_PLAYER].color;
				zoo_board_draw_tile(state, state->board.stats[0].x, state->board.stats[0].y);
				zoo_draw_player_surroundings(state, state->board.stats[0].x, state->board.stats[0].y, 0);
				zoo_draw_player_surroundings(state,
					state->board.stats[0].x - state->input.delta_x,
					state->board.stats[0].y - state->input.delta_y, 0
				);
			}

			// unpause
			state->game_paused = false;
			state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_PAUSED);
			state->current_tick = state->func_random(state, 100);
			state->current_stat_tick = state->board.stat_count + 1;
			state->world.info.is_save = true;
			state->world.info.io_is_save_byte = 1;

			return RETURN_NEXT_CYCLE;
		} else {
			return RETURN_NEXT_CYCLE;
		}
	} else {
		// not paused
		if (state->current_stat_tick <= state->board.stat_count) {
			i = state->current_stat_tick;

			if (state->board.stats[i].cycle != 0
				&& (state->current_tick % state->board.stats[i].cycle) == (i % state->board.stats[i].cycle)
			) {
				// tick self
				zoo_element_defs[
					state->board.tiles[state->board.stats[i].x][state->board.stats[i].y].element
				].tick_func(state, i);

				// if anything on stack...
				if (state->call_stack.call != NULL) {
					state->game_tick_state = 2;
					return RETURN_IMMEDIATE;
				}
			}
GameTickState2:
			state->current_stat_tick++;
		}
	}

GameTickStateDone:
	if (state->game_play_exit_requested) {
		return RETURN_NEXT_CYCLE;
	}

	if (state->current_stat_tick > state->board.stat_count) {
		if (zoo_has_hsecs_elapsed(state, &state->tick_counter, state->tick_duration)) {
			state->current_tick++;
			if (state->current_tick > 420)
				state->current_tick = 1;
			state->current_stat_tick = 0;

			zoo_input_update(&state->input);
			zoo_input_clear(&state->input);

			// Pascal's zero-duration clock check succeeds immediately and the
			// original loop starts the next stat cycle without yielding. Hosts
			// can bound their immediate-tick work per display update if needed.
			if (state->tick_duration == 0)
				return RETURN_IMMEDIATE;
			return RETURN_NEXT_CYCLE;
		} else {
			return RETURN_NEXT_CYCLE;
		}
	}

	return RETURN_IMMEDIATE;
}

void zoo_tick_advance_pit(zoo_state *state) {
	state->time_elapsed += ZOO_PIT_TICK_MS;
	while (state->time_elapsed >= 60000) {
		state->time_elapsed -= 60000;
	}
}

static ZOO_INLINE zoo_tick_retval zoo_tick_inner(zoo_state *state) {
	zoo_tick_retval ret;

	ret = zoo_call_stack_tick(state);
	if (ret != EXIT) {
		return ret;
	}

	switch (state->game_state) {
		default: /* GS_NONE */
			return RETURN_NEXT_CYCLE;
		case GS_TITLE:
		case GS_PLAY:
			return zoo_game_tick(state);
	}
}

zoo_tick_retval zoo_tick(zoo_state *state) {
	zoo_tick_retval ret;

	if (state->error_value) return ERROR;
	ret = zoo_tick_inner(state);
	if (state->error_value) return ERROR;

	return ret;
}
