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
#include "zoo_ui_input.h"

void zoo_input_update(zoo_input_state *state) {
	int i, max_order = -1;
	int move_action = ZOO_ACTION_MAX;
	state->key_pressed = 0;
	if (state->key_buffer_count > 0) {
		uint8_t buffered_count = state->key_buffer_count;
		state->key_pressed = state->key_buffer[state->key_buffer_head];
		if (buffered_count == 1) {
			state->key_buffer_head = (uint8_t)(((uint16_t)state->key_buffer_head + 1) % sizeof(state->key_buffer));
			state->key_buffer_count = 0;
		} else {
			// Pascal's InputUpdate consumes the first key, then assigns
			// Copy(InputKeyBuffer, Length(InputKeyBuffer) - 1, 1). That
			// leaves only the penultimate queued key (and duplicates the
			// first key when exactly two were buffered).
			uint16_t retained_index = ((uint16_t)state->key_buffer_head + buffered_count - 2)
				% sizeof(state->key_buffer);
			state->key_buffer_head = (uint8_t)retained_index;
			state->key_buffer_count = 1;
		}
	}

	for (i = ZOO_ACTION_UP; i <= ZOO_ACTION_DOWN; i++) {
		if (state->actions_down[i] && state->actions_order[i] > max_order) {
			max_order = state->actions_order[i];
			move_action = i;
		}
	}
	/* Keyboard arrows are queued as well as mapped to held actions by the UI
	   adapter. The action latch handles the first press; if multiple taps were
	   queued before a game cycle, later key bytes must still move the player
	   after the one-shot action latch has been cleared. This matches Pascal's
	   InputUpdate, which consumes a direction byte from its key buffer each
	   time it runs. */
	if (move_action == ZOO_ACTION_MAX) {
		switch (state->key_pressed) {
			case ZOO_KEY_UP: case '8': move_action = ZOO_ACTION_UP; break;
			case ZOO_KEY_DOWN: case '2': move_action = ZOO_ACTION_DOWN; break;
			case ZOO_KEY_LEFT: case '4': move_action = ZOO_ACTION_LEFT; break;
			case ZOO_KEY_RIGHT: case '6': move_action = ZOO_ACTION_RIGHT; break;
		}
	}

	switch (move_action) {
		case ZOO_ACTION_UP:
			state->delta_x = 0;
			state->delta_y = -1;
			break;
		case ZOO_ACTION_DOWN:
			state->delta_x = 0;
			state->delta_y = 1;
			break;
		case ZOO_ACTION_LEFT:
			state->delta_x = -1;
			state->delta_y = 0;
			break;
		case ZOO_ACTION_RIGHT:
			state->delta_x = 1;
			state->delta_y = 0;
			break;
		default:
			state->delta_x = 0;
			state->delta_y = 0;
			break;
	}
}

void zoo_input_key_press(zoo_input_state *state, uint16_t key) {
	uint16_t tail;
	if (key == 0 || key > 0xFF || state->key_buffer_count == sizeof(state->key_buffer)) return;
	tail = ((uint16_t)state->key_buffer_head + state->key_buffer_count) % sizeof(state->key_buffer);
	state->key_buffer[tail] = (uint8_t)key;
	state->key_buffer_count++;
}

void zoo_input_clear(zoo_input_state *state) {
	int i;

	// update counts
	for (i = 0; i < ZOO_ACTION_MAX; i++) {
		state->actions_down[i] = false;
	}
}

void zoo_input_tick(zoo_input_state *state) {
	int i;

	// update counts
	for (i = 0; i < ZOO_ACTION_MAX; i++) {
		if (state->actions_held[i]) {
			state->actions_count[i]++;
			if (state->actions_count[i] >= state->repeat_end) {
				state->actions_count[i] = state->repeat_start;
				state->actions_down[i] = true;
			}
		}
	}
}

bool zoo_input_action_pressed(zoo_input_state *state, zoo_input_action action) {
	if (state->actions_down[action]) {
		state->actions_down[action] = false;
		return true;
	} else {
		return false;
	}
}

bool zoo_input_action_held(zoo_input_state *state, zoo_input_action action) {
	return state->actions_held[action];
}

bool zoo_input_shoot_requested(zoo_input_state *state, bool moving) {
	bool requested = state->shoot_tap || (moving && state->actions_held[ZOO_ACTION_SHOOT]);
	state->shoot_tap = false;
	return requested;
}

void zoo_input_dead_player_reset(zoo_input_state *state) {
	state->delta_x = 0;
	state->delta_y = 0;
	zoo_input_action_set(state, ZOO_ACTION_SHOOT, false);
	state->actions_down[ZOO_ACTION_SHOOT] = false;
}

void zoo_input_action_down(zoo_input_state *state, zoo_input_action action) {
	if (!state->actions_held[action]) {
		state->actions_down[action] = true;
		state->actions_held[action] = true;
		state->actions_count[action] = 0;
		state->actions_order[action] = state->pressed_count++;
	}
}

void zoo_input_action_up(zoo_input_state *state, zoo_input_action action) {
	uint8_t old_order;
	int i;

	if (state->actions_held[action]) {
		state->actions_held[action] = false;
		old_order = state->actions_order[action];
		state->actions_order[action] = 0;

		for (i = 0; i < ZOO_ACTION_MAX; i++) {
			if (state->actions_order[i] > old_order) {
				state->actions_order[i]--;
			}
		}

		state->pressed_count--;
	}
}

void zoo_input_action_set(zoo_input_state *state, zoo_input_action action, bool value) {
	if (value) {
		zoo_input_action_down(state, action);
	} else {
		zoo_input_action_up(state, action);
	}
}
