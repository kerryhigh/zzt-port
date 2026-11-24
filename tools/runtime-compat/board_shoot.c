#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zoo_internal.h"

static zoo_state state;
static zoo_video_driver video;
static int write_count;
static int sound_count;
static int16_t last_sound_priority;
static uint8_t last_sound[8];
static int16_t last_sound_length;

static void test_sidebar(zoo_state *test_state, uint16_t flags) {
	(void) test_state;
	(void) flags;
}

static void test_video_write(zoo_video_driver *driver, int16_t x, int16_t y,
	uint8_t color, uint8_t character) {
	(void) driver;
	(void) x;
	(void) y;
	(void) color;
	(void) character;
	write_count++;
}

const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
	[ZOO_E_EMPTY] = {.color = 0x70, .walkable = true, .cycle = -1},
	[ZOO_E_PLAYER] = {.color = 0x0F, .destructible = true, .character = 2, .cycle = -1},
	[ZOO_E_STAR] = {.color = 0x0E, .cycle = -1},
	[ZOO_E_BULLET] = {.color = 0x0F, .cycle = -1},
	[ZOO_E_WATER] = {.color = 0x17, .placeable_on_top = true, .cycle = -1},
	[ZOO_E_DOOR] = {.color = 0x1F, .cycle = -1},
	[ZOO_E_SOLID] = {.color = 0x0F, .cycle = -1},
	[ZOO_E_BREAKABLE] = {.color = 0x0E, .destructible = true, .cycle = -1},
	[ZOO_E_LION] = {.color = 0x0C, .destructible = true, .cycle = -1}
};

#include "../../third_party/libzoo/src/libzoo/zoo_game.c"

void zoo_sound_queue(zoo_sound_state *sound, int16_t priority,
	const uint8_t *data, int16_t len) {
	int16_t i;
	(void) sound;
	sound_count++;
	if (priority < last_sound_priority) return;
	last_sound_priority = priority;
	last_sound_length = len;
	for (i = 0; i < len && i < (int16_t) sizeof(last_sound); i++) {
		last_sound[i] = data[i];
	}
}

void *memcpy(void *dest, const void *src, size_t count) {
	uint8_t *d = (uint8_t *) dest;
	const uint8_t *s = (const uint8_t *) src;
	size_t i;
	for (i = 0; i < count; i++) d[i] = s[i];
	return dest;
}

void *memset(void *dest, int value, size_t count) {
	uint8_t *d = (uint8_t *) dest;
	size_t i;
	for (i = 0; i < count; i++) d[i] = (uint8_t) value;
	return dest;
}

static void reset_test_state(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar;
	state.tick_duration = 4;
	state.board.stat_count = 1;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[10][10].element = ZOO_E_OBJECT;
	state.world.info.health = 0;
	write_count = 0;
	sound_count = 0;
	last_sound_priority = -1;
	last_sound_length = 0;
	memset(last_sound, 0, sizeof(last_sound));
}

int32_t run_board_shoot_checks(void) {
	bool result;

	reset_test_state();
	state.board.tiles[11][10].color = 0x2A;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (!result || state.board.stat_count != 2) return 1;
	if (state.board.tiles[11][10].element != ZOO_E_BULLET
		|| state.board.tiles[11][10].color != 0x0F) return 2;
	if (state.board.stats[2].x != 11 || state.board.stats[2].y != 10
		|| state.board.stats[2].p1 != 0 || state.board.stats[2].step_x != 1
		|| state.board.stats[2].step_y != 0 || state.board.stats[2].p2 != 100) return 3;
	if (state.board.stats[2].under.element != ZOO_E_EMPTY
		|| state.board.stats[2].under.color != 0x2A || write_count != 1 || sound_count != 0) return 4;

	reset_test_state();
	state.board.tiles[10][9].element = ZOO_E_WATER;
	state.board.tiles[10][9].color = 0x17;
	result = zoo_board_shoot(&state, ZOO_E_STAR, 10, 10, 0, -1, 1);
	if (!result || state.board.stat_count != 2) return 5;
	if (state.board.tiles[10][9].element != ZOO_E_STAR
		|| state.board.tiles[10][9].color != 0x1E
		|| state.board.stats[2].under.element != ZOO_E_WATER
		|| state.board.stats[2].step_x != 0 || state.board.stats[2].step_y != -1
		|| state.board.stats[2].p1 != 1 || state.board.stats[2].p2 != 100) return 6;

	reset_test_state();
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (result || state.board.stat_count != 1 || sound_count != 0) return 7;
	if (state.board.tiles[11][10].element != ZOO_E_SOLID) return 8;

	reset_test_state();
	state.board.tiles[11][10].element = ZOO_E_BREAKABLE;
	state.board.tiles[11][10].color = 0x2E;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 1);
	if (!result || state.board.stat_count != 1) return 9;
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2E || sound_count != 1) return 10;
	if (last_sound_priority != 2 || last_sound_length != 2
		|| last_sound[0] != 0x10 || last_sound[1] != 1) return 11;

	reset_test_state();
	state.board.stats[0].x = 11;
	state.board.stats[0].y = 10;
	state.board.tiles[11][10].element = ZOO_E_PLAYER;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (result || sound_count != 0 || state.board.tiles[11][10].element != ZOO_E_PLAYER) return 12;
	state.world.info.health = 100;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 1);
	if (!result || sound_count != 2 || state.board.tiles[11][10].element != ZOO_E_PLAYER
		|| state.board.tiles[11][10].color != 0x7F || state.world.info.health != 90) return 13;
	if (state.board.stat_count != 2 || state.board.stats[2].x != 0
		|| state.board.stats[2].y != 0 || state.board.stats[2].p2 != 20) return 17;
	if (last_sound_priority != 4 || last_sound_length != 8
		|| last_sound[0] != 0x10 || last_sound[1] != 1
		|| last_sound[2] != 0x20 || last_sound[3] != 1
		|| last_sound[4] != 0x13 || last_sound[5] != 1
		|| last_sound[6] != 0x23 || last_sound[7] != 1) return 18;

	reset_test_state();
	state.board.tiles[11][10].element = ZOO_E_LION;
	state.board.tiles[11][10].color = 0x0C;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (!result || state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x0C || sound_count != 1) return 14;
	if (last_sound_priority != 2 || last_sound_length != 2
		|| last_sound[0] != 0x10 || last_sound[1] != 1) return 19;
	reset_test_state();
	state.board.tiles[11][10].element = ZOO_E_DOOR;
	state.board.tiles[11][10].color = 0x2F;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (result || state.board.tiles[11][10].element != ZOO_E_DOOR
		|| state.board.tiles[11][10].color != 0x2F || sound_count != 0) return 15;
	reset_test_state();
	state.world.info.energizer_ticks = 1;
	state.board.tiles[11][10].element = ZOO_E_LION;
	state.board.tiles[11][10].color = 0x0C;
	result = zoo_board_shoot(&state, ZOO_E_BULLET, 10, 10, 1, 0, 0);
	if (result || state.board.tiles[11][10].element != ZOO_E_LION
		|| state.board.tiles[11][10].color != 0x0C || sound_count != 0) return 16;

	return 0;
}
