#include <stddef.h>
#include "zoo_internal.h"

static zoo_state state;
static int16_t random_value;
static int random_calls;

static int16_t scripted_random(zoo_state *unused, int16_t max) {
	(void) unused;
	random_calls++;
	return (max > 0) ? (random_value % max) : 0;
}

#include "../../../third_party/libzoo/src/libzoo/zoo_game.c"

static void clear_test_state(void) {
	unsigned char *bytes = (unsigned char *) &state;
	for (size_t i = 0; i < sizeof(state); i++) bytes[i] = 0;
}

static int check_case(int16_t x, int16_t y, int16_t random,
	int energized, int16_t expected_dx, int16_t expected_dy) {
	int16_t dx = 99, dy = 99;
	clear_test_state();
	random_value = random;
	random_calls = 0;
	state.func_random = scripted_random;
	state.board.stats[0].x = 20;
	state.board.stats[0].y = 20;
	state.world.info.energizer_ticks = energized;
	zoo_calc_direction_seek(&state, x, y, &dx, &dy);
	if (dx != expected_dx || dy != expected_dy) return 1;
	if (random_calls != 1) return 2;
	return 0;
}

int run_direction_seek_checks(void) {
	// Source row first when Random(2) returns zero.
	if (check_case(10, 10, 0, 0, 1, 0) != 0) return 1;
	// Source column first when Random(2) returns one.
	if (check_case(10, 10, 1, 0, 0, 1) != 0) return 2;
	// Same-row alignment forces horizontal pursuit after the random draw.
	if (check_case(10, 20, 1, 0, 1, 0) != 0) return 3;
	// Energizer reverses the chosen direction without another random draw.
	if (check_case(10, 10, 0, 1, -1, 0) != 0) return 4;
	return 0;
}
