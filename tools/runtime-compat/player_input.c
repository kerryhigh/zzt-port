#include <stddef.h>
#include <zoo.h>
#include <zoo_ui_input.h>

static int16_t arrow_burst_output[11];
static int16_t key_burst_output[14];
static int16_t wrapped_burst_output[10];
static int16_t input_length_matrix_output[256 * 2 * 7];
static zoo_input_state key_burst_inputs[5];
static int key_burst_output_count;

uint32_t player_input_arrow_burst_output_ptr(void) {
	return (uint32_t) (uintptr_t) arrow_burst_output;
}

uint32_t player_input_key_burst_output_ptr(void) {
	return (uint32_t) (uintptr_t) key_burst_output;
}

uint32_t player_input_wrapped_burst_output_ptr(void) {
	return (uint32_t) (uintptr_t) wrapped_burst_output;
}

uint32_t player_input_length_matrix_output_ptr(void) {
	return (uint32_t) (uintptr_t) input_length_matrix_output;
}

int run_input_length_matrix_checks(void) {
	int key_length, key_index, stage;
	int16_t *row = input_length_matrix_output;
	static zoo_input_state state;
	for (key_length = 0; key_length <= 255; key_length++) {
		state.delta_x = 0;
		state.delta_y = 0;
		state.key_pressed = 0;
		state.key_buffer_head = 0;
		state.key_buffer_count = 0;
		for (key_index = 0; key_index < key_length; key_index++) {
			uint8_t key = (uint8_t)(((key_index * 73 + 19) % 255) + 1);
			zoo_input_key_press(&state, key);
		}
		for (stage = 1; stage <= 2; stage++) {
			uint8_t remaining = state.key_buffer_count > 0
				? state.key_buffer[state.key_buffer_head] : 0;
			zoo_input_update(&state);
			*row++ = (int16_t) key_length;
			*row++ = (int16_t) stage;
			*row++ = (int16_t) state.key_pressed;
			*row++ = (int16_t) state.key_buffer_count;
			*row++ = state.key_buffer_count > 0
				? (int16_t) state.key_buffer[state.key_buffer_head] : 0;
			*row++ = state.delta_x;
			*row++ = state.delta_y;
			(void) remaining;
		}
	}
	return row == input_length_matrix_output + (256 * 2 * 7) ? 0 : 1;
}

static int run_wrapped_burst_check(void) {
	static const uint8_t burst[] = { 'A', 'B', 'C', 'D', 'E', 'F' };
	static zoo_input_state state;
	int i;
	int16_t *row = wrapped_burst_output;
	for (i = 0; i < 253; i++) {
		zoo_input_key_press(&state, (uint16_t)(((i * 73 + 19) % 255) + 1));
	}
	zoo_input_update(&state);
	zoo_input_update(&state);
	if (state.key_buffer_count != 0 || state.key_buffer_head != 252) return 1;
	for (i = 0; i < (int)(sizeof(burst) / sizeof(burst[0])); i++) {
		zoo_input_key_press(&state, burst[i]);
	}
	for (i = 0; i < 2; i++) {
		zoo_input_update(&state);
		*row++ = state.key_pressed;
		*row++ = state.key_buffer_count;
		*row++ = state.key_buffer_count > 0
			? state.key_buffer[state.key_buffer_head] : 0;
		*row++ = state.delta_x;
		*row++ = state.delta_y;
	}
	return 0;
}

static int capture_key_burst(zoo_input_state *input, const uint8_t *keys, int key_count, int update_count) {
	int i;
	if (input == NULL || keys == NULL || key_count < 1 || update_count < 1
		|| update_count > (int) (sizeof(key_burst_output) / sizeof(key_burst_output[0])) - key_burst_output_count) return 0;
	for (i = 0; i < key_count; i++) zoo_input_key_press(input, keys[i]);
	for (i = 0; i < update_count; i++) {
		zoo_input_update(input);
		key_burst_output[key_burst_output_count++] = input->key_pressed;
	}
	return 1;
}

int run_player_input_checks(void) {
	static zoo_input_state input;
	static zoo_input_state burst4_input;
	static zoo_input_state burst3_input;
	static zoo_input_state burst2_input;
	static zoo_input_state burst1_input;
	static zoo_input_state wrapped_input;
	static zoo_input_state arrow_burst_input;
	static zoo_state cp437_state;
	static zoo_ui_input_state cp437_ui;
	static const uint8_t one[] = { 'A' };
	static const uint8_t two[] = { 'A', 'B' };
	static const uint8_t three[] = { 'A', 'B', 'C' };
	static const uint8_t four[] = { 'A', 'B', 'C', 'D' };
	static const uint8_t repeated_four[] = { 'R', 'R', 'R', 'R' };
	static const uint8_t burst[] = { 'A', 'B', 'C', 'D' };
	int i;
	key_burst_output_count = 0;
	if (!capture_key_burst(&key_burst_inputs[0], one, 1, 2)
		|| !capture_key_burst(&key_burst_inputs[1], two, 2, 3)
		|| !capture_key_burst(&key_burst_inputs[2], three, 3, 3)
		|| !capture_key_burst(&key_burst_inputs[3], four, 4, 3)
		|| !capture_key_burst(&key_burst_inputs[4], repeated_four, 4, 3)
		|| key_burst_output_count != (int) (sizeof(key_burst_output) / sizeof(key_burst_output[0]))) return 22;

	// Pascal's short-string input buffer keeps only the penultimate key after
	// each update. A two-key burst therefore repeats the first key; a four-key
	// burst produces the first and third keys.
	for (i = 0; i < 4; i++) zoo_input_key_press(&burst4_input, burst[i]);
	zoo_input_update(&burst4_input);
	if (burst4_input.key_pressed != 'A' || burst4_input.key_buffer_count != 1 || burst4_input.key_buffer[burst4_input.key_buffer_head] != 'C') return 8;
	zoo_input_update(&burst4_input);
	if (burst4_input.key_pressed != 'C' || burst4_input.key_buffer_count != 0) return 9;

	for (i = 0; i < 3; i++) zoo_input_key_press(&burst3_input, burst[i]);
	zoo_input_update(&burst3_input);
	if (burst3_input.key_pressed != 'A' || burst3_input.key_buffer_count != 1 || burst3_input.key_buffer[burst3_input.key_buffer_head] != 'B') return 13;
	zoo_input_update(&burst3_input);
	if (burst3_input.key_pressed != 'B' || burst3_input.key_buffer_count != 0) return 14;

	zoo_input_key_press(&burst2_input, 'A');
	zoo_input_key_press(&burst2_input, 'B');
	zoo_input_update(&burst2_input);
	if (burst2_input.key_pressed != 'A' || burst2_input.key_buffer_count != 1) return 10;
	zoo_input_update(&burst2_input);
	if (burst2_input.key_pressed != 'A' || burst2_input.key_buffer_count != 0) return 11;

	zoo_input_key_press(&burst1_input, 'A');
	zoo_input_update(&burst1_input);
	if (burst1_input.key_pressed != 'A' || burst1_input.key_buffer_count != 0) return 12;

	wrapped_input.key_buffer_head = 253;
	wrapped_input.key_buffer_count = 4;
	wrapped_input.key_buffer[253] = 'A';
	wrapped_input.key_buffer[254] = 'B';
	wrapped_input.key_buffer[0] = 'C';
	wrapped_input.key_buffer[1] = 'D';
	zoo_input_update(&wrapped_input);
	if (wrapped_input.key_pressed != 'A' || wrapped_input.key_buffer_count != 1 || wrapped_input.key_buffer[wrapped_input.key_buffer_head] != 'C') return 15;
	zoo_input_update(&wrapped_input);
	if (wrapped_input.key_pressed != 'C' || wrapped_input.key_buffer_count != 0) return 16;
	if (run_wrapped_burst_check() != 0) return 23;

	// A rapid pair of arrow taps is a two-byte Pascal InputKeyBuffer burst.
	// The action latch services the first press; the queued second byte must
	// remain movement input after the one-shot latch is cleared.
	zoo_input_key_press(&arrow_burst_input, ZOO_KEY_RIGHT);
	zoo_input_action_down(&arrow_burst_input, ZOO_ACTION_RIGHT);
	zoo_input_action_up(&arrow_burst_input, ZOO_ACTION_RIGHT);
	zoo_input_key_press(&arrow_burst_input, ZOO_KEY_RIGHT);
	zoo_input_action_down(&arrow_burst_input, ZOO_ACTION_RIGHT);
	zoo_input_action_up(&arrow_burst_input, ZOO_ACTION_RIGHT);
	zoo_input_update(&arrow_burst_input);
	arrow_burst_output[0] = arrow_burst_input.key_pressed;
	arrow_burst_output[1] = arrow_burst_input.delta_x;
	arrow_burst_output[2] = arrow_burst_input.delta_y;
	if (arrow_burst_output[0] != ZOO_KEY_RIGHT || arrow_burst_output[1] != 1
		|| arrow_burst_output[2] != 0 || arrow_burst_input.key_buffer_count != 1) return 17;
	zoo_input_clear(&arrow_burst_input);
	zoo_input_update(&arrow_burst_input);
	arrow_burst_output[3] = arrow_burst_input.key_pressed;
	arrow_burst_output[4] = arrow_burst_input.delta_x;
	arrow_burst_output[5] = arrow_burst_input.delta_y;
	if (arrow_burst_output[3] != ZOO_KEY_RIGHT || arrow_burst_output[4] != 1
		|| arrow_burst_output[5] != 0 || arrow_burst_input.key_buffer_count != 0) return 18;
	zoo_input_clear(&arrow_burst_input);
	zoo_input_update(&arrow_burst_input);
	arrow_burst_output[6] = arrow_burst_input.key_pressed;
	arrow_burst_output[7] = arrow_burst_input.delta_x;
	arrow_burst_output[8] = arrow_burst_input.delta_y;
	if (arrow_burst_output[6] != 0 || arrow_burst_output[7] != 0
		|| arrow_burst_output[8] != 0) return 19;

	// Android passes extended CP437 keys through zoo_ui_input_key. The shared
	// bridge must strip the UI-only base before Pascal's byte input queue sees
	// the key, including the upper end of the full 256-byte range.
	zoo_ui_input_key(&cp437_state, &cp437_ui, ZOO_KEY_CP437_BASE + 0x82, true);
	zoo_input_update(&cp437_state.input);
	arrow_burst_output[9] = cp437_state.input.key_pressed;
	if (arrow_burst_output[9] != 0x82) return 20;
	zoo_ui_input_key(&cp437_state, &cp437_ui, ZOO_KEY_CP437_BASE + 0xff, true);
	zoo_input_update(&cp437_state.input);
	arrow_burst_output[10] = cp437_state.input.key_pressed;
	if (arrow_burst_output[10] != 0xff) return 21;

	// Pascal clears movement and held Shift after death but leaves the queued Space key.
	input.delta_x = 1;
	input.delta_y = -1;
	input.actions_held[ZOO_ACTION_SHOOT] = true;
	input.actions_down[ZOO_ACTION_SHOOT] = true;
	input.shoot_tap = true;
	input.pressed_count = 1;
	zoo_input_dead_player_reset(&input);
	if (input.delta_x != 0 || input.delta_y != 0 || input.actions_held[ZOO_ACTION_SHOOT]
			|| input.actions_down[ZOO_ACTION_SHOOT] || input.pressed_count != 0 || !input.shoot_tap) return 1;
	if (!zoo_input_shoot_requested(&input, false) || input.shoot_tap) return 2;
	if (zoo_input_shoot_requested(&input, false)) return 3;

	input.actions_held[ZOO_ACTION_SHOOT] = true;
	if (zoo_input_shoot_requested(&input, false)) return 4;
	if (!zoo_input_shoot_requested(&input, true)) return 5;
	input.actions_held[ZOO_ACTION_SHOOT] = false;
	input.shoot_tap = true;
	if (!zoo_input_shoot_requested(&input, false) || input.shoot_tap) return 6;
	if (zoo_input_shoot_requested(&input, false)) return 7;

	return 0;
}
