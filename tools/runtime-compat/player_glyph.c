#include <stddef.h>
#include <stdint.h>
#include "zoo_internal.h"

#include "../../third_party/libzoo/src/libzoo/zoo_elements.c"

static zoo_state state;

int run_player_glyph_checks(void) {
	uint8_t glyph = 0;

	state.player_character = '\x02';
	zoo_e_player_draw(&state, 0, 0, &glyph);
	if (glyph != '\x02') return 1;

	zoo_e_player_toggle_character(&state);
	if (state.player_character != '\x01') return 2;
	zoo_e_player_draw(&state, 0, 0, &glyph);
	if (glyph != '\x01') return 3;
	zoo_e_player_draw(&state, 0, 0, &glyph);
	if (glyph != '\x01') return 4;

	zoo_e_player_toggle_character(&state);
	if (state.player_character != '\x02') return 5;
	zoo_e_player_reset_character(&state);
	if (state.player_character != '\x02') return 6;

	return 0;
}
