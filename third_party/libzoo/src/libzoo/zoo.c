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
#include <time.h>
#include "zoo_internal.h"
#include "zoo_random.h"

int zoo_world_reload(zoo_state *state) {
	char filename[33];
	zoo_io_handle h;
	int ret;

	if (state->d_io == NULL) {
		state->return_board_id = state->world.info.current_board;
		return ZOO_ERROR_INVAL;
	}

	// Pascal always attempts World.Info.Name + ".ZZT", including when the
	// short string is empty (which means it tries to open ".ZZT").
	strncpy(filename, state->world.info.name, sizeof(filename) - 1);
	filename[sizeof(filename) - 1] = '\0';
	strncat(filename, ".ZZT", sizeof(filename) - strlen(filename) - 1);
	h = state->d_io->func_open_file(state->d_io, filename, MODE_READ);
	if (h.p == NULL) {
		if (h.func_close != NULL) h.func_close(&h);
		state->return_board_id = state->world.info.current_board;
		return ZOO_ERROR_INVAL;
	}

	ret = zoo_world_load(state, &h, false);
	if (h.func_close != NULL) h.func_close(&h);
	state->return_board_id = state->world.info.current_board;

	return ret;
}

int zoo_world_play(zoo_state *state) {
	int ret = 0;

	if (state->world.info.is_save && !state->debug_enabled) {
		ret = zoo_world_reload(state);
	}

	if (!ret) {
		zoo_game_start(state, GS_PLAY);

		zoo_board_change(state, state->return_board_id);
		// Pascal calls BoardEnter after selecting a board to play. Keep it here,
		// outside zoo_game_start, so title and restored-save starts do not reset it.
		zoo_board_enter(state);
		zoo_board_draw_transition(state);

		zoo_redraw(state);
	}

	return ret;
}

int zoo_world_return_title(zoo_state *state) {
	zoo_board_change(state, 0);
	zoo_game_start(state, GS_TITLE);
	zoo_board_draw_transition(state);
	zoo_redraw(state);
	return 0;
}

static int16_t zoo_default_random(zoo_state *state, int16_t max) {
	return zoo_random_next(&state->random_seed, max);
}

static uint32_t zoo_randomize_seed(void) {
	struct timespec now;
	struct tm local_time;

	if (clock_gettime(CLOCK_REALTIME, &now) != 0
			|| localtime_r(&now.tv_sec, &local_time) == NULL) {
		return (uint32_t) time(NULL);
	}

	// DOS INT 21h/AH=2Ch returns CH:CL=hour:minute and DH:DL=second:hundredth.
	return zoo_random_seed_from_dos_time(
		(uint8_t) local_time.tm_hour,
		(uint8_t) local_time.tm_min,
		(uint8_t) local_time.tm_sec,
		(uint8_t) (now.tv_nsec / 10000000L)
	);
}

void* zoo_store_display(zoo_state *state, int16_t x, int16_t y, int16_t width, int16_t height) {
	int16_t ix, iy;
	uint8_t *data, *dp;

	if (state->d_video->func_store_display != NULL) {
		return state->d_video->func_store_display(state->d_video, x, y, width, height);
	} else if (state->d_video->func_read != NULL) {
		data = malloc(width * height * 2);
		if (data != NULL) {
			// if null, assume nop route - this way out of memory isn't fatal
			dp = data;
			for (iy = 0; iy < height; iy++) {
				for (ix = 0; ix < width; ix++, dp += 2) {
					state->d_video->func_read(state->d_video, x + ix, y + iy, dp, dp + 1);
				}
			}
		}

		return data;
	} else {
		// nop route
		return NULL;
	}
}

void zoo_restore_display(zoo_state *state, void *data, int16_t width, int16_t height, int16_t srcx, int16_t srcy, int16_t srcwidth, int16_t srcheight, int16_t dstx, int16_t dsty) {
	int16_t ix, iy;
	uint8_t *data8;

	if (state->d_video->func_restore_display != NULL) {
		return state->d_video->func_restore_display(state->d_video, data, width, height, srcx, srcy, srcwidth, srcheight, dstx, dsty);
	} else if (data != NULL && state->d_video->func_write != NULL) {
		data8 = ((uint8_t *) data) + (srcy * width * 2) + (srcx * 2);
		for (iy = 0; iy < srcheight; iy++) {
			for (ix = 0; ix < srcwidth; ix++, data8 += 2) {
				state->d_video->func_write(state->d_video, dstx + ix, dsty + iy, data8[0], data8[1]);
			}
		}
	} else {
		for (iy = 1; iy <= srcheight; iy++) {
			for (ix = 1; ix <= srcwidth; ix++) {
				zoo_board_draw_tile(state, dstx + ix, dsty + iy);
			}
		}
	}
}

void zoo_free_display(zoo_state *state, void *data) {
	if (data != NULL) {
		free(data);
	}
}

static void zoo_default_video_write(zoo_video_driver *drv, int16_t x, int16_t y, uint8_t a, uint8_t b) {
	// pass
}

static zoo_video_driver d_video_none = {
	zoo_default_video_write
};

static void zoo_default_draw_sidebar(zoo_state *state, uint16_t flags) {
	// pass
}

void zoo_state_init(zoo_state *state) {
	int16_t i, j;
	const int16_t transition_table_size = ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT;
	uint16_t swap;

	memset(state, 0, sizeof(zoo_state));
	state->d_video = &d_video_none;

	state->func_random = zoo_default_random;
	state->func_draw_sidebar = zoo_default_draw_sidebar;

	state->tick_speed = 4;

	zoo_sound_state_init(&(state->sound));
	state->random_seed = zoo_randomize_seed();
	// Pascal builds this shuffled 60x25 table before WorldCreate and reuses it
	// for board-change transitions. Keep the draw count and order identical.
	for (i = 0; i < transition_table_size; i++) {
		state->transition_order[i] = (uint16_t) i;
	}
	for (i = 0; i < transition_table_size; i++) {
		j = zoo_default_random(state, transition_table_size);
		swap = state->transition_order[j];
		state->transition_order[j] = state->transition_order[i];
		state->transition_order[i] = swap;
	}

	state->input.repeat_start = 4;
	state->input.repeat_end = 6;
	state->player_character = 2;

	zoo_world_create(state);
	zoo_game_start(state, GS_TITLE);
}

bool zoo_check_hsecs_elapsed(zoo_state *state, int16_t *hsecs_counter, int16_t hsecs_value) {
	int16_t hsecs_total = (int16_t) (state->time_elapsed / 10);
	int16_t hsecs_diff = (hsecs_total - (*hsecs_counter) + 6000) % 6000;
	return hsecs_diff >= hsecs_value;
}

bool zoo_has_hsecs_elapsed(zoo_state *state, int16_t *hsecs_counter, int16_t hsecs_value) {
	int16_t hsecs_total = (int16_t) (state->time_elapsed / 10);
	int16_t hsecs_diff = (hsecs_total - (*hsecs_counter) + 6000) % 6000;
	if (hsecs_diff >= hsecs_value) {
		*hsecs_counter = hsecs_total;
		return true;
	} else {
		return false;
	}
}

#ifdef ZOO_CONFIG_USE_DOUBLE_FOR_MS
int16_t zoo_hsecs_to_pit_ticks(int16_t hsecs) {
	zoo_time_ms ms = hsecs * 10;
	return (int16_t) ceil((hsecs * 10) / ZOO_PIT_TICK_MS);
}

zoo_time_ms zoo_hsecs_to_pit_ms(int16_t hsecs) {
	return zoo_hsecs_to_pit_ticks(hsecs) * ZOO_PIT_TICK_MS;
}
#else
zoo_time_ms zoo_hsecs_to_pit_ms(int16_t hsecs) {
	zoo_time_ms ms = hsecs * 10 + ZOO_PIT_TICK_MS - 1;
	return ms - (ms % ZOO_PIT_TICK_MS);
}

int16_t zoo_hsecs_to_pit_ticks(int16_t hsecs) {
	zoo_time_ms ms = hsecs * 10 + ZOO_PIT_TICK_MS - 1;
	return ms / ZOO_PIT_TICK_MS;
}
#endif

void zoo_redraw(zoo_state *state) {
	zoo_board_draw(state);
	state->func_draw_sidebar(state, ZOO_SIDEBAR_UPDATE_ALL_REDRAW);
}
