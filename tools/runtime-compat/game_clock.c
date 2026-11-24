#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../third_party/libzoo/src/libzoo/zoo.c"

static zoo_state clock_state;
static int16_t clock_output[14];

static void capture_clock_case(int row, zoo_time_ms now_ms,
	int16_t initial_counter, int16_t duration) {
	int16_t counter = initial_counter;
	clock_state.time_elapsed = now_ms;
	clock_output[row * 2] = (int16_t) zoo_has_hsecs_elapsed(&clock_state, &counter, duration);
	clock_output[row * 2 + 1] = counter;
}

int run_game_clock_checks(void) {
	capture_clock_case(0, 59990, 5990, 10);
	capture_clock_case(1, 59990, 5990, 9);
	capture_clock_case(2, 10, 5999, 2);
	capture_clock_case(3, 50, 1, 5);
	capture_clock_case(4, 60, 1, 5);
	capture_clock_case(5, 10, 1, 0);
	capture_clock_case(6, 900, 100, 25);
	return 0;
}

uint32_t game_clock_output_ptr(void) {
	return (uint32_t) (uintptr_t) clock_output;
}
