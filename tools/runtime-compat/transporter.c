#include <stddef.h>
#include "zoo_internal.h"

static zoo_state state;
static int move_count;
static int push_count;
static int sound_count;
static int16_t moved_from_x, moved_from_y, moved_to_x, moved_to_y;
static int16_t queued_priority;
static int16_t queued_length;
static uint8_t queued_sound[32];

static void clear_test_state(void) {
	unsigned char *bytes = (unsigned char *) &state;
	for (size_t i = 0; i < sizeof(state); i++) bytes[i] = 0;
	move_count = 0;
	push_count = 0;
	sound_count = 0;
	moved_from_x = moved_from_y = moved_to_x = moved_to_y = -1;
	queued_priority = -1;
	queued_length = 0;
	for (size_t i = 0; i < sizeof(queued_sound); i++) queued_sound[i] = 0;
}

zoo_stat *zoo_stat_get(zoo_state *state, int16_t x, int16_t y, int16_t *stat_id) {
	for (int16_t i = 0; i <= state->board.stat_count; i++) {
		if (state->board.stats[i].x == x && state->board.stats[i].y == y) {
			if (stat_id != NULL) *stat_id = i;
			return &state->board.stats[i];
		}
	}
	if (stat_id != NULL) *stat_id = -1;
	return NULL;
}

void zoo_element_move(zoo_state *state, int16_t old_x, int16_t old_y, int16_t new_x, int16_t new_y) {
	(void) state;
	move_count++;
	moved_from_x = old_x;
	moved_from_y = old_y;
	moved_to_x = new_x;
	moved_to_y = new_y;
}

void zoo_element_push(zoo_state *state, int16_t x, int16_t y, int16_t dx, int16_t dy) {
	(void) state;
	(void) x;
	(void) y;
	(void) dx;
	(void) dy;
	push_count++;
}

void zoo_sound_queue(zoo_sound_state *state, int16_t priority, const uint8_t *data, int16_t len) {
	(void) state;
	sound_count++;
	queued_priority = priority;
	queued_length = len;
	for (int16_t i = 0; i < len && i < (int16_t) sizeof(queued_sound); i++) queued_sound[i] = data[i];
}

const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
	[ZOO_E_EMPTY] = {.walkable = true},
	[ZOO_E_PLAYER] = {.walkable = true},
	[ZOO_E_TRANSPORTER] = {.walkable = false},
	[ZOO_E_BOARD_EDGE] = {.walkable = false},
	[ZOO_E_SOLID] = {.walkable = false}
};

#include "transporter_move_production.inc"
#include "transporter_touch_production.inc"

static void set_transporter(int16_t stat_id, int16_t x, int16_t y, int16_t dx, int16_t dy) {
	state.board.stats[stat_id].x = x;
	state.board.stats[stat_id].y = y;
	state.board.stats[stat_id].step_x = dx;
	state.board.stats[stat_id].step_y = dy;
	state.board.tiles[x][y].element = ZOO_E_TRANSPORTER;
}

static int check_touch(int16_t first_dx, bool reverse_exit, bool block_exit) {
	clear_test_state();
	state.board.stat_count = reverse_exit ? 2 : 1;
	state.board.stats[0].x = 10;
	state.board.stats[0].y = 10;
	set_transporter(1, 11, 10, 1, 0);
	state.board.tiles[12][10].element = block_exit ? ZOO_E_SOLID : ZOO_E_EMPTY;
	state.board.tiles[61][10].element = ZOO_E_BOARD_EDGE;
	if (reverse_exit) {
		set_transporter(2, 12, 10, -1, 0);
		state.board.tiles[13][10].element = ZOO_E_EMPTY;
	}
	int16_t dx = first_dx;
	int16_t dy = 0;
	zoo_e_transporter_touch(&state, 11, 10, 0, &dx, &dy);
	if (dx != 0 || dy != 0) return 1;
	if (first_dx != 1 || block_exit) {
		if (move_count != 0 || sound_count != 0 || queued_priority != -1 || queued_length != 0) return 2;
	} else {
		static const uint8_t expected_sound[] = {
			48, 1, 66, 1, 52, 1, 70, 1, 56, 1, 74, 1, 64, 1, 82, 1
		};
		int16_t expected_x = reverse_exit ? 13 : 12;
		if (move_count != 1 || moved_from_x != 10 || moved_from_y != 10
			|| moved_to_x != expected_x || moved_to_y != 10 || sound_count != 1
			|| queued_priority != 3 || queued_length != (int16_t) sizeof(expected_sound)) return 3;
		for (int16_t i = 0; i < queued_length; i++) {
			if (queued_sound[i] != expected_sound[i]) return 4 + i;
		}
	}
	return 0;
}

int run_transporter_compat_checks(void) {
	if (check_touch(1, false, false) != 0) return 1;
	if (check_touch(1, true, false) != 0) return 2;
	if (check_touch(-1, false, false) != 0) return 3;
	if (check_touch(1, false, true) != 0) return 4;
	return 0;
}
