#include <stddef.h>
#include "zoo_internal.h"

static zoo_state state;
static int move_count;
static int16_t moved_x;
static int16_t moved_y;

static void clear_test_state(void) {
	unsigned char *bytes = (unsigned char *) &state;
	for (size_t i = 0; i < sizeof(state); i++) bytes[i] = 0;
}

void zoo_board_draw_tile(zoo_state *state, int16_t x, int16_t y) {
	(void) state;
	(void) x;
	(void) y;
}

void zoo_board_damage_tile(zoo_state *state, int16_t x, int16_t y) {
	(void) state;
	(void) x;
	(void) y;
}

void zoo_board_damage_stat(zoo_state *state, int16_t stat_id) {
	(void) state;
	(void) stat_id;
}

int16_t zoo_stat_get_id(zoo_state *state, int16_t x, int16_t y) {
	if (state->board.stats[0].x == x && state->board.stats[0].y == y) return 0;
	return -1;
}

void zoo_stat_move(zoo_state *state, int16_t stat_id, int16_t new_x, int16_t new_y) {
	zoo_stat *stat = &state->board.stats[stat_id];
	zoo_tile player = state->board.tiles[stat->x][stat->y];
	move_count++;
	moved_x = new_x;
	moved_y = new_y;
	state->board.tiles[stat->x][stat->y].element = ZOO_E_EMPTY;
	state->board.tiles[new_x][new_y] = player;
	stat->x = new_x;
	stat->y = new_y;
}

#include "../../../third_party/libzoo/src/libzoo/zoo_elements.c"

static int check_vertical_push(bool right_is_empty, int16_t expected_x) {
	clear_test_state();
	move_count = 0;
	moved_x = -1;
	moved_y = -1;

	state.board.stats[0].x = 20;
	state.board.stats[0].y = 11;
	state.board.stats[1].x = 20;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_y = 1;
	state.board.stats[1].p1 = 0;
	state.board.stats[1].p2 = 1;
	state.board.stat_count = 1;
	state.board.tiles[20][10].element = ZOO_E_BLINK_WALL;
	state.board.tiles[20][10].color = 7;
	state.board.tiles[20][11].element = ZOO_E_PLAYER;
	state.board.tiles[20][11].color = 0x0F;
	state.board.tiles[21][11].element = right_is_empty ? ZOO_E_EMPTY : ZOO_E_SOLID;
	state.board.tiles[19][11].element = right_is_empty ? ZOO_E_SOLID : ZOO_E_EMPTY;
	state.board.tiles[20][15].element = ZOO_E_SOLID;

	zoo_e_blink_wall_tick(&state, 1);
	if (move_count != 1 || moved_x != expected_x || moved_y != 11) return 1;
	if (state.board.stats[0].x != expected_x || state.board.stats[0].y != 11) return 2;
	if (state.board.tiles[20][11].element != ZOO_E_BLINK_RAY_NS) return 3;
	if (state.board.tiles[expected_x][11].element != ZOO_E_PLAYER) return 4;
	return 0;
}

int run_blink_wall_compat_checks(void) {
	// The Pascal source checks the left cell but accidentally moves right in
	// that fallback branch. Preserve the original ZZT behavior on Android.
	if (check_vertical_push(true, 21) != 0) return 1;
	if (check_vertical_push(false, 21) != 0) return 2;
	return 0;
}
