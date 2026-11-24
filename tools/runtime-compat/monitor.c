#include <stddef.h>
#include <stdint.h>
#include <zoo.h>
#include "zoo_internal.h"

#include "../../third_party/libzoo/src/libzoo/zoo_input.c"
#include "../../third_party/libzoo/src/libzoo/zoo_elements.c"
#include "../../third_party/libzoo/src/ui/zoo_ui_input.c"

static int check_direction_keys(void) {
	static const uint16_t keys[] = {ZOO_KEY_UP, ZOO_KEY_LEFT, ZOO_KEY_RIGHT, ZOO_KEY_DOWN};
	static const int16_t expected_dx[] = {0, -1, 1, 0};
	static const int16_t expected_dy[] = {-1, 0, 0, 1};
	static zoo_state direction_state[4];
	static zoo_ui_input_state direction_ui[4];
	size_t i;

	for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		zoo_ui_input_key_map(&direction_ui[i], ZOO_ACTION_UP, ZOO_KEY_UP);
		zoo_ui_input_key_map(&direction_ui[i], ZOO_ACTION_DOWN, ZOO_KEY_DOWN);
		zoo_ui_input_key_map(&direction_ui[i], ZOO_ACTION_LEFT, ZOO_KEY_LEFT);
		zoo_ui_input_key_map(&direction_ui[i], ZOO_ACTION_RIGHT, ZOO_KEY_RIGHT);
		zoo_ui_input_key(&direction_state[i], &direction_ui[i], keys[i], true);
		zoo_input_update(&direction_state[i].input);
		if (direction_state[i].input.key_pressed != keys[i]
			|| direction_state[i].input.delta_x != expected_dx[i]
			|| direction_state[i].input.delta_y != expected_dy[i]) return (int)i + 1;
	}

	return 0;
}

int run_monitor_compat_checks(void) {
	static const uint16_t exit_keys[] = {27, 'a', 'e', 'h', 'n', 'p', 'q', 'r', 's', 'w', '|'};
	static const uint16_t other_keys[] = {' ', 'b', 'f', 'x', '0', 13, 200};
	static zoo_state state;
	static zoo_ui_input_state ui_input;
	size_t i;
	int direction_result;

	for (i = 0; i < sizeof(exit_keys) / sizeof(exit_keys[0]); i++) {
		state.game_play_exit_requested = false;
		state.input.key_pressed = 0;
		state.input.key_buffer_head = 0;
		state.input.key_buffer_count = 0;
		zoo_input_key_press(&state.input, exit_keys[i]);
		zoo_input_update(&state.input);
		zoo_e_monitor_tick(&state, 0);
		if (!state.game_play_exit_requested) return 1;
	}

	for (i = 0; i < sizeof(other_keys) / sizeof(other_keys[0]); i++) {
		state.game_play_exit_requested = false;
		state.input.key_pressed = 0;
		state.input.key_buffer_head = 0;
		state.input.key_buffer_count = 0;
		zoo_input_key_press(&state.input, other_keys[i]);
		zoo_input_update(&state.input);
		zoo_e_monitor_tick(&state, 0);
		if (state.game_play_exit_requested) return 2;
	}

	state.game_play_exit_requested = false;
	state.input.key_pressed = 0;
	state.input.key_buffer_head = 0;
	state.input.key_buffer_count = 0;
	zoo_input_key_press(&state.input, 'x');
	zoo_input_update(&state.input);
	zoo_e_monitor_tick(&state, 0);
	if (state.game_play_exit_requested || state.input.key_pressed != 'x') return 3;
	zoo_input_key_press(&state.input, 'P');
	zoo_input_update(&state.input);
	zoo_e_monitor_tick(&state, 0);
	if (!state.game_play_exit_requested || state.input.key_pressed != 'P') return 4;

	state.game_play_exit_requested = false;
	state.input.key_pressed = 0;
	state.input.key_buffer_head = 0;
	state.input.key_buffer_count = 0;
	zoo_ui_input_key(&state, &ui_input, ZOO_KEY_CP437_BASE + 'p', true);
	zoo_input_update(&state.input);
	zoo_e_monitor_tick(&state, 0);
	if (!state.game_play_exit_requested || state.input.key_pressed != 'p') return 5;
	direction_result = check_direction_keys();
	if (direction_result != 0) return 5 + direction_result;

	return 0;
}
