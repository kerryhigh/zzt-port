#include <stddef.h>
#include "zoo_internal.h"
#include "zoo_random.h"

static zoo_state state;
static int shot_count;
static uint8_t shot_element[4];
static int16_t shot_x[4];
static int16_t shot_y[4];
static int16_t shot_dx[4];
static int16_t shot_dy[4];
static int16_t shot_source[4];
static bool shot_succeeds;
static int32_t spinning_gun_trace[6][12];
static int spinning_gun_trace_count;

static int16_t zero_random(zoo_state *state, int16_t max) {
	(void) state;
	(void) max;
	return 0;
}

static int16_t pascal_random(zoo_state *state, int16_t max) {
	return zoo_random_next(&state->random_seed, max);
}

void zoo_board_draw_tile(zoo_state *state, int16_t x, int16_t y) {
	(void) state;
	(void) x;
	(void) y;
}

bool zoo_board_shoot(zoo_state *state, uint8_t element, int16_t x, int16_t y,
	int16_t dx, int16_t dy, int16_t param1) {
	(void) state;
	if (shot_count < 4) {
		shot_element[shot_count] = element;
		shot_x[shot_count] = x;
		shot_y[shot_count] = y;
		shot_dx[shot_count] = dx;
		shot_dy[shot_count] = dy;
		shot_source[shot_count] = param1;
	}
	shot_count++;
	return shot_succeeds;
}

void zoo_calc_direction_rnd(zoo_state *state, int16_t *dx, int16_t *dy) {
	// Mirrors GAME.PAS CalcDirectionRnd while keeping this harness focused on
	// the production element tick's call order and shot request.
	*dx = pascal_random(state, 3) - 1;
	if (*dx == 0) {
		*dy = pascal_random(state, 2) * 2 - 1;
	} else {
		*dy = 0;
	}
}

#include "../../../third_party/libzoo/src/libzoo/zoo_elements.c"

static int check_case(int16_t player_x, int16_t player_y,
	int16_t gun_x, int16_t gun_y, int expected_count,
	int16_t expected_dx, int16_t expected_dy) {
	state.func_random = zero_random;
	state.board.stats[0].x = player_x;
	state.board.stats[0].y = player_y;
	state.board.stats[1].x = gun_x;
	state.board.stats[1].y = gun_y;
	state.board.stats[1].p1 = 0;
	state.board.stats[1].p2 = 1;
	shot_count = 0;
	shot_succeeds = false;

	zoo_e_spinning_gun_tick(&state, 1);
	if (shot_count != expected_count) return 1;
	if (expected_count > 0
		&& (shot_dx[0] != expected_dx || shot_dy[0] != expected_dy)) return 2;
	return 0;
}

static int check_pascal_random_case(int16_t player_x, int16_t player_y,
	int16_t gun_x, int16_t gun_y, int16_t p1, int16_t p2,
	int expected_shots, uint8_t expected_element,
	int16_t expected_dx, int16_t expected_dy, uint32_t expected_seed) {
	state.func_random = pascal_random;
	state.random_seed = 0;
	state.board.stats[0].x = player_x;
	state.board.stats[0].y = player_y;
	state.board.stats[1].x = gun_x;
	state.board.stats[1].y = gun_y;
	state.board.stats[1].p1 = p1;
	state.board.stats[1].p2 = p2;
	shot_count = 0;
	shot_succeeds = true;

	zoo_e_spinning_gun_tick(&state, 1);
	if (spinning_gun_trace_count < 6) {
		int32_t *trace = spinning_gun_trace[spinning_gun_trace_count++];
		trace[0] = 1 + shot_count;
		trace[1] = player_x;
		trace[2] = player_y;
		trace[3] = gun_x;
		trace[4] = gun_y;
		trace[5] = (int32_t) state.random_seed;
		trace[6] = shot_count > 0 ? shot_element[0] : 0;
		trace[7] = shot_count > 0 ? shot_x[0] + shot_dx[0] : 0;
		trace[8] = shot_count > 0 ? shot_y[0] + shot_dy[0] : 0;
		trace[9] = shot_count > 0 ? shot_dx[0] : 0;
		trace[10] = shot_count > 0 ? shot_dy[0] : 0;
		trace[11] = shot_count > 0 ? shot_source[0] : 0;
	}
	if (shot_count != expected_shots) return 1;
	if (state.random_seed != expected_seed) return 2;
	if (expected_shots > 0 && (shot_element[0] != expected_element
		|| shot_x[0] != gun_x || shot_y[0] != gun_y
		|| shot_dx[0] != expected_dx || shot_dy[0] != expected_dy
		|| shot_source[0] != 1)) return 3;
	return 0;
}

int run_spinning_gun_checks(void) {
	spinning_gun_trace_count = 0;

	// A failed vertical shot must not try a horizontal shot when rows differ,
	// even if the gun happens to be within two columns of the player.
	if (check_case(10, 20, 12, 10, 1, 0, 1) != 0) return 1;

	// A gun on the same row must try horizontally, even when columns differ.
	if (check_case(20, 10, 10, 12, 1, 1, 0) != 0) return 2;

	// Direct Pascal traces use RandSeed zero, exercising the real TP 5.5 draw
	// sequence, aimed/random branches, star selection, and the zero-rate gate.
	if (check_pascal_random_case(10, 20, 12, 10, 8, 1, 1,
		ZOO_E_BULLET, 0, 1, 134775814U) != 0) return 3;
	if (check_pascal_random_case(20, 10, 10, 12, 8, 1, 1,
		ZOO_E_BULLET, 1, 0, 134775814U) != 0) return 4;
	if (check_pascal_random_case(10, 20, 12, 10, 0, 1, 1,
		ZOO_E_BULLET, 1, 0, 3698175007U) != 0) return 5;
	if (check_pascal_random_case(20, 10, 10, 12, 0, 1, 1,
		ZOO_E_BULLET, 1, 0, 3698175007U) != 0) return 6;
	if (check_pascal_random_case(10, 20, 12, 10, 0, 129, 1,
		ZOO_E_STAR, 1, 0, 3698175007U) != 0) return 7;
	if (check_pascal_random_case(10, 20, 12, 10, 8, 0, 0,
		0, 0, 0, 1U) != 0) return 8;

	return 0;
}

int32_t get_spinning_gun_trace_count(void) {
	return spinning_gun_trace_count;
}

const int32_t *get_spinning_gun_trace(void) {
	return &spinning_gun_trace[0][0];
}
