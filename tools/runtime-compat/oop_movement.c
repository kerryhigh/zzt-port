#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zoo_internal.h"
#include "zoo_editor_brd.h"
#include "zoo_high_scores.h"
#include "zoo_random.h"

static zoo_state state;
static zoo_video_driver video;
static int video_write_count;
static int sidebar_update_count;
static uint16_t sidebar_update_flags;
static int centipede_random_count;
static int16_t centipede_random_max[4];
static int creature_random_count;
static int16_t creature_random_max[4];
static int16_t creature_random_value[4];
static bool force_elapsed_for_tick_check;
static bool integrated_fixed_clock_enabled;
static int16_t integrated_fixed_clock_hsecs;
static int16_t slime_compat_outputs[5 * 40];
static int16_t bomb_tick_outputs[5 * 23];
static int16_t conveyor_tick_outputs[2 * 22];
static int16_t star_tick_outputs[4 * 13];
static int16_t tiger_tick_outputs[2 * 23];
static int16_t centipede_tick_outputs[4 * 38];
static int16_t ruffian_tick_outputs[3 * 22];
static int16_t bear_tick_outputs[3 * 5];
static int16_t shark_tick_outputs[2 * 7];
static int16_t lion_tick_outputs[3 * 12];
static int16_t pusher_tick_outputs[2 * 61];
static int16_t duplicator_tick_outputs[5 * 61];
static int16_t element_draw_outputs[34 + (4 * 4 * 7)];
static int16_t object_tick_outputs[4 * 9];
static int16_t object_touch_outputs[4 * 8];
static int16_t transporter_tick_write_count;
static int16_t transporter_tick_write_x;
static int16_t transporter_tick_write_y;
static int16_t transporter_tick_write_color;
static int16_t transporter_tick_write_character;
static int16_t message_timer_outputs[7 * 138];
static int16_t blink_wall_tick_outputs[2 * 21];
static int16_t scroll_tick_outputs[256 * 2];
static int16_t scroll_touch_outputs[2 * 155];
static int16_t player_movement_outputs[4 * 17];
static int16_t player_shoot_outputs[5 * 24];
static int16_t player_key_outputs[2 * 12];
static int16_t player_death_outputs[2 * 18];
static int16_t player_paused_move_output[3 * 14];
static uint8_t message_row_chars[60];
static uint8_t message_row_colors[60];
static bool capture_scroll_touch_lines;
static int scroll_touch_line_count;
static char scroll_touch_lines[2][51];
#define INTEGRATED_GAMEPLAY_CHECKPOINT_CAPACITY 160
#define INTEGRATED_GAMEPLAY_MAX_FIELDS 8192
#define INTEGRATED_GAMEPLAY_SOUND_EVENT_CAPACITY 64
#define INTEGRATED_GAMEPLAY_SOUND_EVENT_STRIDE 516
#define INTEGRATED_GAMEPLAY_CELL_EVENT_CAPACITY 16384
#define INTEGRATED_GAMEPLAY_CELL_EVENT_STRIDE 4
#define INTEGRATED_GAMEPLAY_SAVE_CAPACITY 65536
#define INTEGRATED_TITLE_RESTORE_FAILURE_FIELD_CAPACITY 4000
static int32_t integrated_gameplay_snapshots[
	INTEGRATED_GAMEPLAY_CHECKPOINT_CAPACITY][INTEGRATED_GAMEPLAY_MAX_FIELDS];
static int32_t integrated_gameplay_snapshot_lengths[INTEGRATED_GAMEPLAY_CHECKPOINT_CAPACITY];
static int16_t integrated_gameplay_checkpoint_headers[INTEGRATED_GAMEPLAY_CHECKPOINT_CAPACITY * 2];
static int32_t integrated_gameplay_snapshot_count_value;
static int16_t integrated_gameplay_trace[24 * 15];
static int32_t integrated_gameplay_sound_events[
	INTEGRATED_GAMEPLAY_SOUND_EVENT_CAPACITY][INTEGRATED_GAMEPLAY_SOUND_EVENT_STRIDE];
static int32_t integrated_gameplay_sound_event_count_value;
static int32_t integrated_gameplay_sound_event_overflow_value;
static bool integrated_gameplay_sound_capture_enabled;
static int32_t integrated_gameplay_cell_events[
	INTEGRATED_GAMEPLAY_CELL_EVENT_CAPACITY][INTEGRATED_GAMEPLAY_CELL_EVENT_STRIDE];
static int32_t integrated_gameplay_cell_context_events[
	INTEGRATED_GAMEPLAY_CELL_EVENT_CAPACITY][8];
static int32_t integrated_gameplay_cell_event_count_value;
static int32_t integrated_gameplay_cell_event_overflow_value;
static bool integrated_gameplay_cell_capture_enabled;
static bool integrated_gameplay_cell_coalesce_idempotent_writes;
static int32_t integrated_gameplay_cell_idempotent_write_count_value;
static uint8_t integrated_gameplay_cell_screen_valid[60 * 25];
static uint8_t integrated_gameplay_cell_screen_color[60 * 25];
static uint8_t integrated_gameplay_cell_screen_character[60 * 25];
static uint8_t integrated_gameplay_save_bytes[INTEGRATED_GAMEPLAY_SAVE_CAPACITY];
static int32_t integrated_gameplay_save_length_value;
static int32_t integrated_gameplay_save_result_value;
static int32_t integrated_gameplay_save_overflow_value;
static uint8_t integrated_gameplay_reload_input_bytes[INTEGRATED_GAMEPLAY_SAVE_CAPACITY];
static int32_t integrated_gameplay_reload_result_value;
static uint8_t integrated_gameplay_roundtrip_save_bytes[INTEGRATED_GAMEPLAY_SAVE_CAPACITY];
static int32_t integrated_gameplay_roundtrip_save_length_value;
static int32_t integrated_gameplay_roundtrip_save_result_value;
static int32_t integrated_gameplay_roundtrip_save_overflow_value;
static uint8_t integrated_demo_world_bytes[INTEGRATED_GAMEPLAY_SAVE_CAPACITY];
static zoo_io_driver integrated_title_world_io;
static int32_t integrated_title_world_length;
static int32_t integrated_title_restore_failure_fields[INTEGRATED_TITLE_RESTORE_FAILURE_FIELD_CAPACITY];
static int32_t integrated_title_restore_failure_field_count_value;
static int32_t integrated_title_restore_failure_error_value;
static int32_t integrated_end_play_output[7][5];
static int32_t integrated_title_quit_output[3][2];
static uint8_t integrated_editor_brd_bytes[INTEGRATED_GAMEPLAY_SAVE_CAPACITY];
static int32_t integrated_editor_brd_length_value;
static uint8_t integrated_f10_bad_world_bytes[512];
static int32_t integrated_f10_bad_world_load_result_value;
static zoo_high_score_table integrated_f10_high_score_table;
static uint8_t integrated_f10_high_score_save_bytes[ZOO_HIGH_SCORE_TABLE_BYTES];
static bool integrated_gameplay_initial_paused = true;
static uint16_t integrated_gameplay_post_loop_key_pressed;
static bool integrated_schedule_enabled;
static int32_t integrated_elapsed_nanos;
static int16_t integrated_configured_tick_speed;
static const char *integrated_normalized_input_tape;
static int16_t integrated_normalized_input_tape_length;
static int16_t integrated_normalized_input_tape_position;
static bool integrated_normalized_input_at_update;
static bool integrated_torch_action_pending;
static bool integrated_cycle_clock_enabled;
static bool integrated_capture_first_board_change_checkpoint;
static bool integrated_board_change_checkpoint_captured;
static bool integrated_board_change_checkpoint_failed;
static bool integrated_pause_transition_checkpoint_pending;

static int capture_integrated_gameplay_snapshot(int checkpoint_index, int16_t checkpoint_kind, int16_t checkpoint_id);

static void capture_player_shoot_state(int case_index, int target_x, int target_y, bool has_bullet);
static void capture_player_death_state(int case_index);

static int16_t test_zero_random(zoo_state *test_state, int16_t max) {
	(void) test_state;
	(void) max;
	return 0;
}

static int16_t test_four_random(zoo_state *test_state, int16_t max) {
	(void) test_state;
	return (max > 4) ? 4 : (max > 0 ? (int16_t) (max - 1) : 0);
}

static int16_t integrated_gameplay_random(zoo_state *test_state, int16_t max) {
	return (int16_t) zoo_random_next(&test_state->random_seed, max);
}

static zoo_io_handle integrated_title_world_open_file(zoo_io_driver *io,
		const char *filename, zoo_io_mode mode) {
	(void) io;
	if (mode != MODE_READ || strcmp(filename, "DEMO.ZZT") != 0
			|| integrated_title_world_length <= 0) {
		return zoo_io_open_file_empty();
	}
	return zoo_io_open_file_mem(integrated_demo_world_bytes,
		(size_t) integrated_title_world_length, MODE_READ);
}

static int16_t test_centipede_random(zoo_state *test_state, int16_t max) {
	(void) test_state;
	if (centipede_random_count < (int) (sizeof(centipede_random_max) / sizeof(centipede_random_max[0]))) {
		centipede_random_max[centipede_random_count] = max;
	}
	centipede_random_count++;
	return 0;
}

static int16_t test_creature_random(zoo_state *test_state, int16_t max) {
	int index = creature_random_count++;
	(void) test_state;
	if (index < (int) (sizeof(creature_random_max) / sizeof(creature_random_max[0]))) {
		creature_random_max[index] = max;
		return creature_random_value[index];
	}
	return 0;
}

static void test_video_write(zoo_video_driver *driver, int16_t x, int16_t y,
	uint8_t color, uint8_t character) {
	(void) driver;
	#ifdef ZZT_INTEGRATED_GAMEPLAY_ONLY
	if (integrated_gameplay_cell_capture_enabled && x >= 0 && x < 60 && y >= 0 && y < 25) {
		int cell = (y * 60) + x;
		bool capture = true;
		if (integrated_gameplay_cell_coalesce_idempotent_writes) {
			if (integrated_gameplay_cell_screen_valid[cell]
				&& integrated_gameplay_cell_screen_color[cell] == color
				&& integrated_gameplay_cell_screen_character[cell] == character) {
				integrated_gameplay_cell_idempotent_write_count_value++;
				capture = false;
			} else {
				integrated_gameplay_cell_screen_valid[cell] = 1;
				integrated_gameplay_cell_screen_color[cell] = color;
				integrated_gameplay_cell_screen_character[cell] = character;
			}
		}
		if (!capture) {
			/* The screen already contains this exact attribute/glyph pair. */
		} else if (integrated_gameplay_cell_event_count_value < INTEGRATED_GAMEPLAY_CELL_EVENT_CAPACITY) {
			int32_t *event = integrated_gameplay_cell_events[
				integrated_gameplay_cell_event_count_value++];
			event[0] = x;
			event[1] = y;
			event[2] = color;
			event[3] = character;
			int32_t *context = integrated_gameplay_cell_context_events[
				integrated_gameplay_cell_event_count_value - 1];
			context[0] = state.world.info.current_board;
			context[1] = state.board.stats[0].x;
			context[2] = state.board.stats[0].y;
			context[3] = state.game_paused ? 1 : 0;
			context[4] = state.game_paused_blink ? 1 : 0;
			context[5] = state.game_tick_state;
			context[6] = state.current_stat_tick;
			context[7] = state.current_tick;
		} else {
			integrated_gameplay_cell_event_overflow_value = 1;
		}
	}
	#else
	(void) x;
	(void) y;
	(void) color;
	(void) character;
	#endif
	video_write_count++;
}

static void test_transporter_tick_write(zoo_video_driver *driver, int16_t x, int16_t y,
	uint8_t color, uint8_t character) {
	(void) driver;
	transporter_tick_write_count++;
	transporter_tick_write_x = x;
	transporter_tick_write_y = y;
	transporter_tick_write_color = color;
	transporter_tick_write_character = character;
}

static void test_message_video_write(zoo_video_driver *driver, int16_t x, int16_t y,
	uint8_t color, uint8_t character) {
	(void) driver;
	if (y == 24 && x >= 0 && x < 60) {
		message_row_chars[x] = character;
		message_row_colors[x] = color;
	}
	video_write_count++;
}

static void test_sidebar_update(zoo_state *test_state, uint16_t flags) {
	(void) test_state;
	sidebar_update_count++;
	sidebar_update_flags = flags;
}

static void capture_window_append(const char *text) {
	if (capture_scroll_touch_lines && scroll_touch_line_count < 2) {
		strncpy(scroll_touch_lines[scroll_touch_line_count], text, 50);
		scroll_touch_lines[scroll_touch_line_count][50] = '\0';
		scroll_touch_line_count++;
	}
}

#include "../../third_party/libzoo/src/libzoo/zoo_game_io.c"
#ifdef ZZT_INTEGRATED_GAMEPLAY_ONLY
#include "../../third_party/libzoo/src/libzoo/zoo_callstack.c"
#include "../../third_party/libzoo/src/libzoo/zoo_editor_brd.c"
#include "../../third_party/libzoo/src/libzoo/zoo_high_scores.c"
#endif
#include "../../third_party/libzoo/src/libzoo/zoo_io.c"
#define zoo_board_change zoo_board_change_actual
#include "../../third_party/libzoo/src/libzoo/zoo_game.c"
#undef zoo_board_change

/* Keep the Pascal BoardChange checkpoint at the same point in the touch path:
   after the target board opens and before the board-edge callback clears dx. */
void zoo_board_change_actual(zoo_state *test_state, int16_t board_id);
void zoo_board_change(zoo_state *test_state, int16_t board_id) {
	zoo_board_change_actual(test_state, board_id);
	if (integrated_capture_first_board_change_checkpoint && test_state == &state) {
		integrated_capture_first_board_change_checkpoint = false;
		if (capture_integrated_gameplay_snapshot(
				integrated_gameplay_snapshot_count_value, 'T', 2)) {
			integrated_board_change_checkpoint_captured = true;
		} else {
			integrated_board_change_checkpoint_failed = true;
		}
	}
}

#define zoo_window_append zoo_window_append_actual
#define zoo_window_append_pascal zoo_window_append_pascal_actual
#include "../../third_party/libzoo/src/libzoo/zoo_window.c"
#undef zoo_window_append_pascal
#undef zoo_window_append

void zoo_window_append(zoo_text_window *window, const char *text) {
	capture_window_append(text);
	zoo_window_append_actual(window, text);
}

void zoo_window_append_pascal(zoo_state *test_state, zoo_text_window *window, const char *text) {
	capture_window_append(text);
	zoo_window_append_pascal_actual(test_state, window, text);
}

void zoo_window_open(zoo_state *test_state, zoo_text_window *window) {
	test_state->object_window = *window;
	test_state->object_window_request = true;
}

#include "../../third_party/libzoo/src/libzoo/zoo_elements.c"
#define zoo_input_update zoo_input_update_actual
#include "../../third_party/libzoo/src/libzoo/zoo_input.c"
#undef zoo_input_update

/* Test-only equivalent of the Pascal InputUpdate tape override. The hook is
   inactive for ordinary probes and changes only deterministic test input. */
void zoo_input_update(zoo_input_state *input) {
	char command;
	zoo_input_update_actual(input);
	if (!integrated_normalized_input_at_update
			|| integrated_normalized_input_tape == NULL
			|| integrated_normalized_input_tape_position >= integrated_normalized_input_tape_length) return;
	command = integrated_normalized_input_tape[integrated_normalized_input_tape_position];
	if (integrated_pause_transition_checkpoint_pending && state.game_paused && command == 'R') return;
	integrated_normalized_input_tape_position++;
	input->delta_x = 0;
	input->delta_y = 0;
	switch (command) {
		case 'U': input->delta_y = -1; break;
		case 'D': input->delta_y = 1; break;
		case 'L': input->delta_x = -1; break;
		case 'R': input->delta_x = 1; break;
		case 'T':
			/* Preserve Pascal's observable key byte in snapshots while translating
			   the same logical input to LibZoo's mapped torch action. */
			input->key_pressed = 'T';
			integrated_torch_action_pending = true;
			break;
		case 'N': input->key_pressed = 0; break;
		case 'P': input->key_pressed = 'P'; break;
	}
}
#include "../../third_party/libzoo/src/libzoo/zoo_oop.c"
#ifdef ZZT_INTEGRATED_GAMEPLAY_ONLY
#define zoo_sound_queue zoo_sound_queue_untraced
#endif
#include "../../third_party/libzoo/src/libzoo/zoo_sound.c"
#ifdef ZZT_INTEGRATED_GAMEPLAY_ONLY
#undef zoo_sound_queue
void zoo_sound_queue(zoo_sound_state *sound, int16_t priority, const uint8_t *data, int16_t len) {
	int32_t *event;
	int index;
	zoo_sound_queue_untraced(sound, priority, data, len);
	if (!integrated_gameplay_sound_capture_enabled) return;
	if (integrated_gameplay_sound_event_count_value >= INTEGRATED_GAMEPLAY_SOUND_EVENT_CAPACITY
			|| len < 0 || len > 255 || sound->buffer_len < 0 || sound->buffer_len > 255) {
		integrated_gameplay_sound_event_overflow_value = 1;
		return;
	}
	event = integrated_gameplay_sound_events[integrated_gameplay_sound_event_count_value++];
	event[0] = priority;
	event[1] = len;
	for (index = 0; index < len; index++) event[2 + index] = data[index];
	event[257] = sound->current_priority;
	event[258] = sound->buffer_pos + 1;
	event[259] = sound->buffer_len;
	event[260] = sound->is_playing ? 1 : 0;
	for (index = 0; index < sound->buffer_len; index++) event[261 + index] = sound->buffer[index];
}
#endif
#ifdef ZZT_INTEGRATED_GAMEPLAY_ONLY
/* Exercise the same public lifecycle calls used by the frontends. */
#define zoo_has_hsecs_elapsed zoo_has_hsecs_elapsed_host
#include "../../third_party/libzoo/src/libzoo/zoo.c"
#undef zoo_has_hsecs_elapsed
#endif

bool zoo_has_hsecs_elapsed(zoo_state *test_state, int16_t *counter, int16_t value) {
	if (!force_elapsed_for_tick_check) return false;
	if (integrated_fixed_clock_enabled) {
		int16_t difference = (int16_t) ((integrated_fixed_clock_hsecs - *counter + 6000) % 6000);
		if (difference < value) return false;
		*counter = integrated_fixed_clock_hsecs;
	} else if (integrated_schedule_enabled) {
		int16_t current_hsecs = (int16_t) ((integrated_elapsed_nanos / 10000000) % 6000);
		int16_t difference = (int16_t) ((current_hsecs - *counter + 6000) % 6000);
		integrated_elapsed_nanos += 54924578;
		/* Existing zero-speed replay fixtures deliberately model immediate
		   polling; positive-speed F07 uses the staged Pascal clock threshold. */
		if (integrated_configured_tick_speed == 0) {
			*counter = current_hsecs;
			return true;
		}
		if (difference < value) return false;
		*counter = current_hsecs;
	} else {
		*counter = (int16_t) (test_state->time_elapsed / 10);
	}
	(void) value;
	return true;
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

void *memmove(void *dest, const void *src, size_t count) {
	uint8_t *d = (uint8_t *) dest;
	const uint8_t *s = (const uint8_t *) src;
	size_t i;
	if (d < s) {
		for (i = 0; i < count; i++) d[i] = s[i];
	} else if (d > s) {
		for (i = count; i > 0; i--) d[i - 1] = s[i - 1];
	}
	return dest;
}

size_t strlen(const char *text) {
	const char *end = text;
	while (*end != '\0') end++;
	return (size_t) (end - text);
}

size_t strnlen(const char *text, size_t limit) {
	size_t length = 0;
	while (length < limit && text[length] != '\0') length++;
	return length;
}

int strcmp(const char *left, const char *right) {
	while (*left == *right && *left != '\0') {
		left++;
		right++;
	}
	return (int) (uint8_t) *left - (int) (uint8_t) *right;
}

int strncmp(const char *left, const char *right, size_t count) {
	size_t i;
	for (i = 0; i < count; i++) {
		uint8_t a = (uint8_t) left[i];
		uint8_t b = (uint8_t) right[i];
		if (a != b || a == 0) return (int) a - (int) b;
	}
	return 0;
}

int memcmp(const void *left, const void *right, size_t count) {
	const uint8_t *a = (const uint8_t *) left;
	const uint8_t *b = (const uint8_t *) right;
	size_t i;
	for (i = 0; i < count; i++) {
		if (a[i] != b[i]) return (int) a[i] - (int) b[i];
	}
	return 0;
}

char *strncpy(char *dest, const char *src, size_t count) {
	size_t i = 0;
	while (i < count && src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}
	while (i < count) dest[i++] = '\0';
	return dest;
}

char *strncat(char *dest, const char *src, size_t count) {
	size_t end = strlen(dest);
	size_t i = 0;
	while (i < count && src[i] != '\0') {
		dest[end + i] = src[i];
		i++;
	}
	dest[end + i] = '\0';
	return dest;
}

char *strchr(const char *text, int value) {
	while (*text != '\0') {
		if ((uint8_t) *text == (uint8_t) value) return (char *) text;
		text++;
	}
	return value == 0 ? (char *) text : NULL;
}

static void reset_movement_state(const char *script) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.stats[1].data = (char *) script;
	state.board.stats[1].data_len = (int16_t) strlen(script);
	state.board.tiles[10][10].element = ZOO_E_OBJECT;
	state.board.tiles[10][10].color = 0x2A;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x31;
	video_write_count = 0;
}

static int check_move(const char *script) {
	int16_t position = 0;
	reset_movement_state(script);
	zoo_oop_execute(&state, 1, &position, "Test");
	if (state.board.stats[1].x != 11) return 100 + state.board.stats[1].x;
	if (state.board.stats[1].y != 10) return 200 + state.board.stats[1].y;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 2;
	if (state.board.tiles[11][10].element != ZOO_E_OBJECT
		|| state.board.tiles[11][10].color != 0x0A) return 3;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x31) return 4;
	if (video_write_count != 2) return 5;
	if (state.board.stats[1].cycle != 0) return 6;
	return 0;
}

static int check_push_move(const char *script) {
	int16_t position = 0;
	reset_movement_state(script);
	state.board.tiles[11][10].element = ZOO_E_BOULDER;
	state.board.tiles[11][10].color = 0x0E;
	state.board.tiles[12][10].element = ZOO_E_EMPTY;
	state.board.tiles[12][10].color = 0x70;
	zoo_oop_execute(&state, 1, &position, "Test");
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 2;
	if (state.board.tiles[11][10].element != ZOO_E_OBJECT
		|| state.board.tiles[11][10].color != 0x0A) return 3;
	if (state.board.tiles[12][10].element != ZOO_E_BOULDER
		|| state.board.tiles[12][10].color != 0x0E) return 4;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x0E) return 5;
	if (video_write_count != 4 || state.board.stats[1].cycle != 0) return 6;
	return 0;
}

static int check_boulder_chain(const char *script) {
	int16_t position = 0;
	reset_movement_state(script);
	state.board.tiles[11][10].element = ZOO_E_BOULDER;
	state.board.tiles[11][10].color = 0x0E;
	state.board.tiles[12][10].element = ZOO_E_BOULDER;
	state.board.tiles[12][10].color = 0x0E;
	state.board.tiles[13][10].element = ZOO_E_EMPTY;
	state.board.tiles[13][10].color = 0x70;
	zoo_oop_execute(&state, 1, &position, "Test");
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 2;
	if (state.board.tiles[11][10].element != ZOO_E_OBJECT
		|| state.board.tiles[11][10].color != 0x0A) return 3;
	if (state.board.tiles[12][10].element != ZOO_E_BOULDER
		|| state.board.tiles[13][10].element != ZOO_E_BOULDER) return 4;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x0E) return 5;
	if (video_write_count != 6 || state.board.stats[1].cycle != 0) return 6;
	return 0;
}

static int check_blocked_boulder_chain(const char *script, bool is_try) {
	int16_t position = 0;
	reset_movement_state(script);
	state.board.tiles[11][10].element = ZOO_E_BOULDER;
	state.board.tiles[11][10].color = 0x0E;
	state.board.tiles[12][10].element = ZOO_E_SOLID;
	state.board.tiles[12][10].color = 0x0F;
	state.board.tiles[13][10].element = ZOO_E_EMPTY;
	state.board.tiles[13][10].color = 0x70;
	zoo_oop_execute(&state, 1, &position, "Test");
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 10) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_OBJECT
		|| state.board.tiles[11][10].element != ZOO_E_BOULDER
		|| state.board.tiles[12][10].element != ZOO_E_SOLID) return 2;
	if (video_write_count != 0) return 3;
	if (is_try) {
		if (state.board.stats[1].cycle != 5) return 4;
	} else if (state.board.stats[1].cycle != 0 || position != 0) {
		return 5;
	}
	return 0;
}

static int check_slider_move(const char *script, uint8_t slider_element, int16_t dx, int16_t dy, bool should_push) {
	int16_t position = 0;
	int16_t target_x = (int16_t) (10 + dx);
	int16_t target_y = (int16_t) (10 + dy);
	int16_t next_x = (int16_t) (target_x + dx);
	int16_t next_y = (int16_t) (target_y + dy);
	reset_movement_state(script);
	state.board.tiles[target_x][target_y].element = slider_element;
	state.board.tiles[target_x][target_y].color = 0x0E;
	state.board.tiles[next_x][next_y].element = ZOO_E_EMPTY;
	state.board.tiles[next_x][next_y].color = 0x70;
	zoo_oop_execute(&state, 1, &position, "Test");
	if (should_push) {
		if (state.board.stats[1].x != target_x || state.board.stats[1].y != target_y) return 1;
		if (state.board.tiles[10][10].element != ZOO_E_EMPTY
			|| state.board.tiles[10][10].color != 0x70) return 2;
		if (state.board.tiles[target_x][target_y].element != ZOO_E_OBJECT
			|| state.board.tiles[target_x][target_y].color != 0x0A) return 3;
		if (state.board.tiles[next_x][next_y].element != slider_element
			|| state.board.tiles[next_x][next_y].color != 0x0E) return 4;
		if (state.board.stats[1].under.element != ZOO_E_EMPTY
			|| state.board.stats[1].under.color != 0x0E) return 5;
		if (video_write_count != 4 || state.board.stats[1].cycle != 0) return 6;
	} else {
		if (state.board.stats[1].x != 10 || state.board.stats[1].y != 10) return 7;
		if (state.board.tiles[10][10].element != ZOO_E_OBJECT
			|| state.board.tiles[10][10].color != 0x2A) return 8;
		if (state.board.tiles[target_x][target_y].element != slider_element
			|| state.board.tiles[next_x][next_y].element != ZOO_E_EMPTY) return 9;
		if (video_write_count != 0 || state.board.stats[1].cycle != 5) return 10;
	}
	return 0;
}

static void reset_duplicator_limit_state(int16_t stat_count) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.sound.current_priority = -1;
	state.board.stat_count = stat_count;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_x = 1;
	state.board.stats[1].p1 = 5;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.stats[2].x = 11;
	state.board.stats[2].y = 10;
	state.board.stats[2].cycle = 4;
	state.board.stats[2].under.element = ZOO_E_EMPTY;
	state.board.stats[2].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_DUPLICATOR;
	state.board.tiles[10][10].color = 0x0E;
	state.board.tiles[11][10].element = ZOO_E_OBJECT;
	state.board.tiles[11][10].color = 0x2A;
	state.board.tiles[9][10].element = ZOO_E_EMPTY;
	state.board.tiles[9][10].color = 0x70;
	video_write_count = 0;
}

static int check_duplicator_stat_limit(void) {
	reset_duplicator_limit_state(ZOO_MAX_STAT - 1);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	if (state.board.stat_count != ZOO_MAX_STAT) return 1;
	if (state.board.tiles[9][10].element != ZOO_E_OBJECT
		|| state.board.tiles[9][10].color != 0x2A) return 2;
	if (state.board.stats[ZOO_MAX_STAT].x != 9
		|| state.board.stats[ZOO_MAX_STAT].y != 10
		|| state.board.stats[ZOO_MAX_STAT].cycle != 4) return 3;

	reset_duplicator_limit_state(ZOO_MAX_STAT);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	if (state.board.stat_count != ZOO_MAX_STAT) return 4;
	if (state.board.tiles[9][10].element != ZOO_E_EMPTY
		|| state.board.tiles[9][10].color != 0x70) return 5;
	return 0;
}

static void reset_pusher_chain_state(bool blocked) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 2;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_x = 1;
	state.board.stats[1].follower = -1;
	state.board.stats[1].leader = -1;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.stats[2].x = 11;
	state.board.stats[2].y = 10;
	state.board.stats[2].step_x = 1;
	state.board.stats[2].follower = -1;
	state.board.stats[2].leader = -1;
	state.board.stats[2].under.element = ZOO_E_EMPTY;
	state.board.stats[2].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_PUSHER;
	state.board.tiles[10][10].color = 0x0F;
	state.board.tiles[11][10].element = ZOO_E_PUSHER;
	state.board.tiles[11][10].color = 0x0F;
	state.board.tiles[12][10].element = ZOO_E_BOULDER;
	state.board.tiles[12][10].color = 0x0E;
	state.board.tiles[13][10].element = blocked ? ZOO_E_SOLID : ZOO_E_EMPTY;
	state.board.tiles[13][10].color = blocked ? 0x0F : 0x70;
	state.board.tiles[14][10].element = ZOO_E_EMPTY;
	state.board.tiles[14][10].color = 0x70;
	video_write_count = 0;
}

static int check_pusher_chain(void) {
	reset_pusher_chain_state(false);
	zoo_element_defs[ZOO_E_PUSHER].tick_func(&state, 2);
	if (state.board.stats[1].x != 11 || state.board.stats[2].x != 12) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].element != ZOO_E_PUSHER
		|| state.board.tiles[12][10].element != ZOO_E_PUSHER
		|| state.board.tiles[13][10].element != ZOO_E_BOULDER) return 2;

	reset_pusher_chain_state(true);
	zoo_element_defs[ZOO_E_PUSHER].tick_func(&state, 2);
	if (state.board.stats[1].x != 10 || state.board.stats[2].x != 11) return 3;
	if (state.board.tiles[10][10].element != ZOO_E_PUSHER
		|| state.board.tiles[11][10].element != ZOO_E_PUSHER
		|| state.board.tiles[12][10].element != ZOO_E_BOULDER
		|| state.board.tiles[13][10].element != ZOO_E_SOLID) return 4;
	return 0;
}

static void capture_push_tick_state(int16_t *out, int16_t case_id, int16_t extra_stat_id) {
	int16_t stat_id, x, i;
	*out++ = case_id;
	*out++ = state.board.stat_count;
	for (stat_id = 1; stat_id <= 3; stat_id++) {
		const zoo_stat *stat = stat_id == 3 && extra_stat_id > 0
			? &state.board.stats[extra_stat_id] : &state.board.stats[stat_id];
		bool present = stat_id <= state.board.stat_count
			&& (stat_id != 3 || extra_stat_id > 0);
		if (present) {
			*out++ = stat->x;
			*out++ = stat->y;
			*out++ = stat->step_x;
			*out++ = stat->step_y;
			*out++ = stat->cycle;
			*out++ = stat->p1;
			*out++ = stat->p2;
			*out++ = stat->p3;
			*out++ = stat->follower;
			*out++ = stat->leader;
			*out++ = stat->under.element;
			*out++ = stat->under.color;
		} else {
			for (i = 0; i < 12; i++) *out++ = -1;
		}
	}
	for (x = 9; x <= 14; x++) {
		*out++ = state.board.tiles[x][10].element;
		*out++ = state.board.tiles[x][10].color;
	}
	*out++ = state.sound.current_priority;
	*out++ = state.sound.is_playing ? 1 : 0;
	*out++ = state.sound.buffer_len;
	for (i = 0; i < 8; i++) {
		*out++ = i < state.sound.buffer_len ? state.sound.buffer[i] : -1;
	}
}

static void reset_duplicator_capture_state(int16_t stat_count, uint8_t source_element) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.sound.current_priority = -1;
	state.board.stat_count = stat_count;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_x = 1;
	state.board.stats[1].cycle = 9;
	state.board.stats[1].follower = -1;
	state.board.stats[1].leader = -1;
	state.board.stats[1].p1 = 5;
	state.board.stats[1].p2 = 2;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_DUPLICATOR;
	state.board.tiles[10][10].color = 0x0E;
	state.board.tiles[11][10].element = source_element;
	state.board.tiles[11][10].color = 0x2A;
	state.board.tiles[9][10].element = ZOO_E_EMPTY;
	state.board.tiles[9][10].color = 0x70;
	if (source_element == ZOO_E_OBJECT) {
		state.board.stats[2].x = 11;
		state.board.stats[2].y = 10;
		state.board.stats[2].cycle = 4;
		state.board.stats[2].p1 = 6;
		state.board.stats[2].p2 = 7;
		state.board.stats[2].p3 = 8;
		state.board.stats[2].step_x = -1;
		state.board.stats[2].step_y = 1;
		state.board.stats[2].follower = -1;
		state.board.stats[2].leader = -1;
		state.board.stats[2].under.element = ZOO_E_EMPTY;
		state.board.stats[2].under.color = 0x70;
	}
	video_write_count = 0;
}

int32_t run_duplicator_tick_capture_checks(void) {
	reset_duplicator_capture_state(2, ZOO_E_OBJECT);
	state.board.stats[1].p1 = 4;
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	capture_push_tick_state(&duplicator_tick_outputs[0], 1, 0);

	reset_duplicator_capture_state(2, ZOO_E_OBJECT);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	capture_push_tick_state(&duplicator_tick_outputs[61], 2, 3);

	reset_duplicator_capture_state(1, ZOO_E_BOULDER);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	capture_push_tick_state(&duplicator_tick_outputs[2 * 61], 3, 0);

	reset_duplicator_capture_state(ZOO_MAX_STAT - 1, ZOO_E_OBJECT);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	capture_push_tick_state(&duplicator_tick_outputs[3 * 61], 4, ZOO_MAX_STAT);

	reset_duplicator_capture_state(ZOO_MAX_STAT, ZOO_E_OBJECT);
	zoo_element_defs[ZOO_E_DUPLICATOR].tick_func(&state, 1);
	capture_push_tick_state(&duplicator_tick_outputs[4 * 61], 5, 0);
	return 0;
}

int32_t run_pusher_tick_capture_checks(void) {
	reset_pusher_chain_state(false);
	state.sound.current_priority = -1;
	zoo_element_defs[ZOO_E_PUSHER].tick_func(&state, 2);
	capture_push_tick_state(&pusher_tick_outputs[0], 1, 0);

	reset_pusher_chain_state(true);
	state.sound.current_priority = -1;
	zoo_element_defs[ZOO_E_PUSHER].tick_func(&state, 2);
	capture_push_tick_state(&pusher_tick_outputs[61], 2, 0);
	return 0;
}

uint32_t pusher_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) pusher_tick_outputs;
}

uint32_t duplicator_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) duplicator_tick_outputs;
}

static void reset_bullet_tick_state(uint8_t target_element, uint8_t target_color) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 1;
	state.current_stat_tick = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_x = 1;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_BULLET;
	state.board.tiles[10][10].color = 0x2E;
	state.sound.current_priority = -1;
	state.sound.is_playing = false;
	state.sound.block_queueing = false;
	state.board.tiles[11][10].element = target_element;
	state.board.tiles[11][10].color = target_color;
	state.board.tiles[9][10].element = ZOO_E_EMPTY;
	state.board.tiles[9][10].color = 0x70;
	video_write_count = 0;
}

static int check_bullet_tick(void) {
	reset_bullet_tick_state(ZOO_E_EMPTY, 0x70);
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stats[1].x != 11 || state.board.stats[1].step_x != 1) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 2;
	if (state.board.tiles[11][10].element != ZOO_E_BULLET
		|| state.board.tiles[11][10].color != 0x0E) return 3;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x70) return 4;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 5;

	reset_bullet_tick_state(ZOO_E_WATER, 0x4D);
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stats[1].x != 11) return 5;
	if (state.board.tiles[11][10].element != ZOO_E_BULLET
		|| state.board.tiles[11][10].color != 0x4E) return 6;
	if (state.board.stats[1].under.element != ZOO_E_WATER
		|| state.board.stats[1].under.color != 0x4D) return 7;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 8;

	reset_bullet_tick_state(ZOO_E_RICOCHET, 0x0F);
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stats[1].x != 9 || state.board.stats[1].step_x != -1) return 8;
	if (state.board.tiles[11][10].element != ZOO_E_RICOCHET
		|| state.board.tiles[9][10].element != ZOO_E_BULLET) return 9;
	if (state.sound.current_priority != 1 || state.sound.buffer_len != 2
		|| state.sound.buffer[0] != 249 || state.sound.buffer[1] != 1) return 22;

	reset_bullet_tick_state(ZOO_E_SOLID, 0x0F);
	state.board.tiles[10][11].element = ZOO_E_RICOCHET;
	state.board.tiles[10][11].color = 0x0F;
	state.board.tiles[10][9].color = 0x70;
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stats[1].x != 10) return 10;
	if (state.board.stats[1].y != 9) return 11;
	if (state.board.stats[1].step_x != 0) return 12;
	if (state.board.stats[1].step_y != -1) return 13;
	if (state.board.tiles[10][11].element != ZOO_E_RICOCHET
		|| state.board.tiles[10][9].element != ZOO_E_BULLET) return 14;
	if (state.sound.current_priority != 1 || state.sound.buffer_len != 2
		|| state.sound.buffer[0] != 249 || state.sound.buffer[1] != 1) return 23;

	reset_bullet_tick_state(ZOO_E_SOLID, 0x0F);
	state.board.tiles[10][9].element = ZOO_E_RICOCHET;
	state.board.tiles[10][9].color = 0x0F;
	state.board.tiles[10][11].color = 0x70;
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stats[1].x != 10) return 15;
	if (state.board.stats[1].y != 11) return 16;
	if (state.board.stats[1].step_x != 0) return 17;
	if (state.board.stats[1].step_y != 1) return 18;
	if (state.board.tiles[10][9].element != ZOO_E_RICOCHET
		|| state.board.tiles[10][11].element != ZOO_E_BULLET) return 19;
	if (state.sound.current_priority != 1 || state.sound.buffer_len != 2
		|| state.sound.buffer[0] != 249 || state.sound.buffer[1] != 1) return 24;

	reset_bullet_tick_state(ZOO_E_SOLID, 0x0F);
	zoo_element_defs[ZOO_E_BULLET].tick_func(&state, 1);
	if (state.board.stat_count != 0 || state.current_stat_tick != 0) return 20;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].element != ZOO_E_SOLID) return 21;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 25;
	return 0;
}

static void reset_conveyor_state(uint8_t conveyor_element, int boulder_index, uint8_t boulder_color) {
	static const int8_t diagonal_x[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
	static const int8_t diagonal_y[8] = {1, 1, 1, 0, -1, -1, -1, 0};
	int i;
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = conveyor_element;
	state.board.tiles[10][10].color = 0x07;
	for (i = 0; i < 8; i++) {
		state.board.tiles[10 + diagonal_x[i]][10 + diagonal_y[i]].element = ZOO_E_EMPTY;
		state.board.tiles[10 + diagonal_x[i]][10 + diagonal_y[i]].color = 0x70;
	}
	state.board.tiles[10 + diagonal_x[boulder_index]][10 + diagonal_y[boulder_index]].element = ZOO_E_BOULDER;
	state.board.tiles[10 + diagonal_x[boulder_index]][10 + diagonal_y[boulder_index]].color = boulder_color;
	video_write_count = 0;
}

static int check_conveyor_rotation(void) {
	reset_conveyor_state(ZOO_E_CONVEYOR_CW, 0, 0x1E);
	zoo_element_defs[ZOO_E_CONVEYOR_CW].tick_func(&state, 1);
	if (state.board.tiles[9][10].element != ZOO_E_BOULDER
		|| state.board.tiles[9][10].color != 0x1E) return 1;
	if (state.board.tiles[9][11].element != ZOO_E_EMPTY) return 2;
	/* The adjacent empty tile rotates into the boulder's old slot. */
	if (state.board.tiles[9][11].color != 0x70) return 3;

	reset_conveyor_state(ZOO_E_CONVEYOR_CCW, 7, 0x2E);
	zoo_element_defs[ZOO_E_CONVEYOR_CCW].tick_func(&state, 1);
	if (state.board.tiles[9][11].element != ZOO_E_BOULDER
		|| state.board.tiles[9][11].color != 0x2E) return 4;
	if (state.board.tiles[9][10].element != ZOO_E_EMPTY) return 5;
	if (state.board.tiles[9][10].color != 0x70) return 6;
	return 0;
}

static void capture_conveyor_tick_state(int16_t case_id) {
	int16_t *out = &conveyor_tick_outputs[(case_id - 1) * 22];
	int16_t i;
	*out++ = case_id;
	*out++ = state.board.stat_count;
	*out++ = state.board.stats[1].x;
	*out++ = state.board.stats[1].y;
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[10][10].color;
	for (i = 0; i < 8; i++) {
		int16_t x = 10 + zoo_diagonal_delta_x[i];
		int16_t y = 10 + zoo_diagonal_delta_y[i];
		*out++ = state.board.tiles[x][y].element;
		*out++ = state.board.tiles[x][y].color;
	}
}

int32_t run_conveyor_tick_capture_checks(void) {
	reset_conveyor_state(ZOO_E_CONVEYOR_CW, 0, 0x1E);
	zoo_element_defs[ZOO_E_CONVEYOR_CW].tick_func(&state, 1);
	capture_conveyor_tick_state(1);

	reset_conveyor_state(ZOO_E_CONVEYOR_CCW, 7, 0x2E);
	zoo_element_defs[ZOO_E_CONVEYOR_CCW].tick_func(&state, 1);
	capture_conveyor_tick_state(2);
	return 0;
}

uint32_t conveyor_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) conveyor_tick_outputs;
}

static int check_bomb_countdown(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_random = test_zero_random;
	state.board.stat_count = 1;
	state.current_stat_tick = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].p1 = 2;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_BOMB;
	state.board.tiles[10][10].color = 0x0E;
	video_write_count = 0;

	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	if (state.board.stats[1].p1 != 1 || state.board.stat_count != 1) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_BOMB) return 2;
	if (state.sound.buffer_len != 12 || state.sound.buffer[0] != 96) return 3;

	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	if (state.board.stat_count != 0 || state.current_stat_tick != 1) return 4;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 5;
	return 0;
}

static void reset_bomb_tick_capture_state(int16_t fuse) {
	int16_t x, y;
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_random = test_four_random;
	state.current_stat_tick = 1;
	for (x = 5; x <= 15; x++) {
		for (y = 5; y <= 15; y++) {
			state.board.tiles[x][y].element = ZOO_E_SOLID;
			state.board.tiles[x][y].color = 0x46;
		}
	}
	state.board.stat_count = 1;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].cycle = 6;
	state.board.stats[1].p1 = fuse;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_BOMB;
	state.board.tiles[10][10].color = 0x28;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x50;
	state.sound.current_priority = -1;
	video_write_count = 0;
}

static void capture_bomb_tick_state(int16_t case_id) {
	int16_t *out = &bomb_tick_outputs[(case_id - 1) * 23];
	int i;
	*out++ = case_id;
	*out++ = state.board.stat_count;
	if (state.board.stat_count > 0) {
		*out++ = state.board.stats[1].p1;
		*out++ = state.board.stats[1].under.element;
		*out++ = state.board.stats[1].under.color;
	} else {
		*out++ = -1;
		*out++ = -1;
		*out++ = -1;
	}
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[10][10].color;
	*out++ = state.board.tiles[11][10].element;
	*out++ = state.board.tiles[11][10].color;
	*out++ = state.sound.current_priority;
	*out++ = state.sound.buffer_len;
	for (i = 0; i < 12; i++) {
		*out++ = i < state.sound.buffer_len ? state.sound.buffer[i] : -1;
	}
}

int32_t run_bomb_tick_capture_checks(void) {
	reset_bomb_tick_capture_state(0);
	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	capture_bomb_tick_state(1);

	reset_bomb_tick_capture_state(3);
	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	capture_bomb_tick_state(2);
	state.sound.current_priority = -1;
	state.sound.buffer_len = 0;
	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	capture_bomb_tick_state(3);
	state.sound.current_priority = -1;
	state.sound.buffer_len = 0;
	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	capture_bomb_tick_state(4);

	reset_bomb_tick_capture_state(-1);
	zoo_element_defs[ZOO_E_BOMB].tick_func(&state, 1);
	capture_bomb_tick_state(5);
	return 0;
}

uint32_t bomb_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) bomb_tick_outputs;
}

static int check_bomb_touch(void) {
	static const char expected_message[] = "Bomb activated!";
	static const uint8_t expected_sound[] = { 0x30, 0x01, 0x35, 0x01, 0x40, 0x01, 0x45, 0x01, 0x50, 0x01 };
	int16_t dx = 1;
	int16_t dy = 0;
	int i;
	reset_movement_state("");
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = 0x1F;
	state.board.stats[1].p1 = 0;
	state.board.stats[1].data = NULL;
	state.board.stats[1].data_len = 0;
	state.board.tiles[10][10].element = ZOO_E_BOMB;
	state.board.tiles[10][10].color = 0x28;
	state.tick_duration = 4;
	state.sound.current_priority = -1;

	zoo_element_defs[ZOO_E_BOMB].touch_func(&state, 10, 10, 0, &dx, &dy);
	if (state.board.stats[1].p1 != 9) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_BOMB
		|| state.board.tiles[10][10].color != 0x28) return 2;
	if (state.board.info.io_message_length != sizeof(expected_message) - 1)
		return 100 + state.board.info.io_message_length;
	for (i = 0; i < (int) sizeof(expected_message) - 1; i++) {
		if (state.board.info.message[i] != expected_message[i]) return 20 + i;
	}
	if (state.board.info.message[sizeof(expected_message) - 1] != '\0') return 40;
	if (state.board.stat_count != 2 || state.board.stats[2].x != 0
		|| state.board.stats[2].y != 0 || state.board.stats[2].p2 != 40) return 3;
	if (state.sound.current_priority != 4 || state.sound.buffer_len != sizeof(expected_sound)) return 4;
	for (i = 0; i < (int) sizeof(expected_sound); i++) {
		if (state.sound.buffer[i] != expected_sound[i]) return 50 + i;
	}
	if (dx != 1 || dy != 0) return 5;

	reset_movement_state("");
	state.board.stats[1].p1 = 9;
	state.board.stats[1].data = NULL;
	state.board.stats[1].data_len = 0;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_BOMB;
	state.board.tiles[10][10].color = 0x28;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x31;
	state.sound.current_priority = -1;
	dx = 1;
	dy = 0;

	zoo_element_defs[ZOO_E_BOMB].touch_func(&state, 10, 10, 0, &dx, &dy);
	if (state.board.stats[1].p1 != 9 || state.board.stats[1].x != 11
		|| state.board.stats[1].y != 10) return 6;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 7;
	if (state.board.tiles[11][10].element != ZOO_E_BOMB
		|| state.board.tiles[11][10].color != 0x08) return 8;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x31) return 9;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 10;
	if (dx != 1 || dy != 0) return 11;

	reset_movement_state("");
	state.board.stats[1].p1 = 9;
	state.board.stats[1].data = NULL;
	state.board.stats[1].data_len = 0;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_BOMB;
	state.board.tiles[10][10].color = 0x28;
	state.board.tiles[11][10].element = ZOO_E_NORMAL;
	state.board.tiles[11][10].color = 0x0E;
	state.sound.current_priority = -1;

	zoo_element_defs[ZOO_E_BOMB].touch_func(&state, 10, 10, 0, &dx, &dy);
	if (state.board.stats[1].p1 != 9 || state.board.stats[1].x != 10
		|| state.board.stats[1].y != 10) return 12;
	if (state.board.tiles[10][10].element != ZOO_E_BOMB
		|| state.board.tiles[10][10].color != 0x28) return 13;
	if (state.board.tiles[11][10].element != ZOO_E_NORMAL
		|| state.board.tiles[11][10].color != 0x0E) return 14;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 15;
	if (dx != 1 || dy != 0) return 16;
	return 0;
}

static int check_slime_reproduction_interval(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].p1 = 2;
	state.board.stats[1].p2 = 2;
	state.board.stats[1].cycle = zoo_element_defs[ZOO_E_SLIME].cycle;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_SLIME;
	state.board.tiles[10][10].color = 0x2A;
	state.board.tiles[10][9].element = ZOO_E_EMPTY;
	state.board.tiles[10][9].color = 0x70;
	state.board.tiles[10][11].element = ZOO_E_EMPTY;
	state.board.tiles[10][11].color = 0x70;
	state.board.tiles[9][10].element = ZOO_E_SOLID;
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	video_write_count = 0;

	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	if (state.board.stat_count != 2) return 1;
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 9
		|| state.board.stats[1].p1 != 0 || state.board.stats[1].p2 != 2) return 2;
	if (state.board.stats[2].x != 10 || state.board.stats[2].y != 11
		|| state.board.stats[2].p1 != 0 || state.board.stats[2].p2 != 2) return 3;
	if (state.board.tiles[10][10].element != ZOO_E_BREAKABLE
		|| state.board.tiles[10][9].element != ZOO_E_SLIME
		|| state.board.tiles[10][11].element != ZOO_E_SLIME) return 4;

	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 2);
	if (state.board.stat_count != 2 || state.board.stats[2].p1 != 1
		|| state.board.stats[2].p2 != 2) return 5;
	return 0;
}

static void reset_slime_compat_state(int16_t p1, int16_t p2, uint8_t open_mask) {
	static const int16_t neighbor_x[4] = {10, 10, 9, 11};
	static const int16_t neighbor_y[4] = {9, 11, 10, 10};
	int i;

	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.board.stat_count = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].cycle = zoo_element_defs[ZOO_E_SLIME].cycle;
	state.board.stats[1].p1 = p1;
	state.board.stats[1].p2 = p2;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_SLIME;
	state.board.tiles[10][10].color = 0x2A;
	for (i = 0; i < 4; i++) {
		zoo_tile *tile = &state.board.tiles[neighbor_x[i]][neighbor_y[i]];
		if ((open_mask & (1u << i)) != 0) {
			tile->element = ZOO_E_EMPTY;
			tile->color = (uint8_t) (0x30 + i);
		} else {
			tile->element = ZOO_E_SOLID;
			tile->color = (uint8_t) (0x40 + i);
		}
	}
	video_write_count = 0;
}

static void capture_slime_compat_state(int16_t case_id) {
	static const int16_t tile_x[5] = {10, 10, 10, 9, 11};
	static const int16_t tile_y[5] = {10, 9, 11, 10, 10};
	int16_t *out = &slime_compat_outputs[(case_id - 1) * 40];
	int i;
	*out++ = case_id;
	*out++ = state.board.stat_count;
	for (i = 1; i <= 4; i++) {
		if (i <= state.board.stat_count) {
			const zoo_stat *stat = &state.board.stats[i];
			*out++ = stat->x;
			*out++ = stat->y;
			*out++ = stat->cycle;
			*out++ = stat->p1;
			*out++ = stat->p2;
			*out++ = stat->under.element;
			*out++ = stat->under.color;
		} else {
			int field;
			for (field = 0; field < 7; field++) *out++ = -1;
		}
	}
	for (i = 0; i < 5; i++) {
		const zoo_tile *tile = &state.board.tiles[tile_x[i]][tile_y[i]];
		*out++ = tile->element;
		*out++ = tile->color;
	}
}

int32_t run_slime_tick_capture_checks(void) {
	reset_slime_compat_state(1, 2, 0x0F);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	capture_slime_compat_state(1);

	reset_slime_compat_state(2, 2, 0x0F);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	capture_slime_compat_state(2);

	reset_slime_compat_state(3, 2, 0x08);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	capture_slime_compat_state(3);

	reset_slime_compat_state(2, 2, 0x00);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	capture_slime_compat_state(4);

	reset_slime_compat_state(2, 2, 0x03);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 1);
	zoo_element_defs[ZOO_E_SLIME].tick_func(&state, 2);
	capture_slime_compat_state(5);
	return 0;
}

uint32_t slime_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) slime_compat_outputs;
}

static void reset_centipede_state(int chain_length) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_random = test_centipede_random;
	state.board.stat_count = (int16_t) chain_length;
	state.board.stats[0].x = 2;
	state.board.stats[0].y = 2;
	state.board.tiles[2][2].element = ZOO_E_PLAYER;
	state.board.tiles[2][2].color = zoo_element_defs[ZOO_E_PLAYER].color;
	for (int i = 1; i <= chain_length; i++) {
		zoo_stat *stat = &state.board.stats[i];
		stat->x = (uint8_t) (10 - (i - 1));
		stat->y = 10;
		stat->cycle = 1;
		stat->step_x = 1;
		stat->p1 = 4;
		stat->p2 = 0;
		stat->leader = (i == 1) ? -1 : (int16_t) (i - 1);
		stat->follower = (i == chain_length) ? -1 : (int16_t) (i + 1);
		stat->under.element = ZOO_E_EMPTY;
		stat->under.color = 0x70;
		state.board.tiles[stat->x][stat->y].element =
			(i == 1) ? ZOO_E_CENTIPEDE_HEAD : ZOO_E_CENTIPEDE_SEGMENT;
		state.board.tiles[stat->x][stat->y].color =
			zoo_element_defs[state.board.tiles[stat->x][stat->y].element].color;
	}
	centipede_random_count = 0;
	memset(centipede_random_max, 0, sizeof(centipede_random_max));
	video_write_count = 0;
}

static int check_centipede_chain_and_reversal(void) {
	reset_centipede_state(2);
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x70;
	zoo_element_defs[ZOO_E_CENTIPEDE_HEAD].tick_func(&state, 1);
	if (centipede_random_count != 1 || centipede_random_max[0] != 10) return 1;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10
		|| state.board.stats[1].follower != 2 || state.board.stats[1].leader != -1) return 2;
	if (state.board.stats[2].x != 10 || state.board.stats[2].y != 10
		|| state.board.stats[2].leader != 1 || state.board.stats[2].follower != -1
		|| state.board.stats[2].step_x != 1 || state.board.stats[2].step_y != 0
		|| state.board.stats[2].p1 != state.board.stats[1].p1
		|| state.board.stats[2].p2 != state.board.stats[1].p2) return 3;
	if (state.board.tiles[11][10].element != ZOO_E_CENTIPEDE_HEAD
		|| state.board.tiles[10][10].element != ZOO_E_CENTIPEDE_SEGMENT
		|| state.board.tiles[9][10].element != ZOO_E_EMPTY) return 4;
	if (state.board.tiles[8][10].element != ZOO_E_EMPTY
		|| state.board.tiles[8][10].color != 0
		|| state.board.tiles[9][10].color != 0x70
		|| state.board.tiles[10][10].color != 0x0F
		|| state.board.tiles[11][10].color != 0x0F) return 10;

	reset_centipede_state(3);
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	state.board.tiles[10][9].element = ZOO_E_SOLID;
	state.board.tiles[10][11].element = ZOO_E_SOLID;
	zoo_element_defs[ZOO_E_CENTIPEDE_HEAD].tick_func(&state, 1);
	if (centipede_random_count != 3 || centipede_random_max[0] != 10
		|| centipede_random_max[1] != 2 || centipede_random_max[2] != 2) return 5;
	if (state.board.stats[1].x != 10 || state.board.stats[1].leader != 2
		|| state.board.stats[1].follower != -1) return 6;
	if (state.board.stats[2].x != 9 || state.board.stats[2].leader != 3
		|| state.board.stats[2].follower != 1) return 7;
	if (state.board.stats[3].x != 8 || state.board.stats[3].leader != 2
		|| state.board.stats[3].follower != 2) return 8;
	if (state.board.tiles[10][10].element != ZOO_E_CENTIPEDE_SEGMENT
		|| state.board.tiles[9][10].element != ZOO_E_CENTIPEDE_SEGMENT
		|| state.board.tiles[8][10].element != ZOO_E_CENTIPEDE_HEAD) return 9;
	if (state.board.tiles[8][10].color != ZOO_COLOR_CHOICE_ON_BLACK
		|| state.board.tiles[9][10].color != ZOO_COLOR_CHOICE_ON_BLACK
		|| state.board.tiles[10][10].color != ZOO_COLOR_CHOICE_ON_BLACK
		|| state.board.tiles[11][10].color != 0) return 11;

	reset_centipede_state(1);
	state.board.stats[1].leader = -1;
	state.board.stats[1].follower = -1;
	state.board.tiles[10][10].element = ZOO_E_CENTIPEDE_SEGMENT;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].color;
	zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].tick_func(&state, 1);
	if (state.board.stats[1].leader != -2
		|| state.board.tiles[10][10].element != ZOO_E_CENTIPEDE_SEGMENT
		|| state.board.tiles[10][10].color != ZOO_COLOR_CHOICE_ON_BLACK) return 12;
	zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].tick_func(&state, 1);
	if (state.board.stats[1].leader != -2
		|| state.board.tiles[10][10].element != ZOO_E_CENTIPEDE_HEAD
		|| state.board.tiles[10][10].color != ZOO_COLOR_CHOICE_ON_BLACK) return 13;
	return 0;
}

static void capture_centipede_tick_state(int16_t case_id) {
	int16_t *out = &centipede_tick_outputs[(case_id - 1) * 38];
	int16_t i, stat_id;
	*out++ = case_id;
	*out++ = centipede_random_count;
	for (i = 0; i < 3; i++) {
		*out++ = i < centipede_random_count ? centipede_random_max[i] : -1;
	}
	*out++ = state.board.stat_count;
	for (stat_id = 1; stat_id <= 3; stat_id++) {
		if (stat_id <= state.board.stat_count) {
			zoo_stat *stat = &state.board.stats[stat_id];
			*out++ = stat->x;
			*out++ = stat->y;
			*out++ = stat->step_x;
			*out++ = stat->step_y;
			*out++ = stat->leader;
			*out++ = stat->follower;
			*out++ = stat->p1;
			*out++ = stat->p2;
		} else {
			for (i = 0; i < 8; i++) *out++ = -1;
		}
	}
	for (i = 8; i <= 11; i++) {
		*out++ = state.board.tiles[i][10].element;
		*out++ = state.board.tiles[i][10].color;
	}
}

int32_t run_centipede_tick_capture_checks(void) {
	reset_centipede_state(2);
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x70;
	zoo_element_defs[ZOO_E_CENTIPEDE_HEAD].tick_func(&state, 1);
	capture_centipede_tick_state(1);

	reset_centipede_state(3);
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	state.board.tiles[10][9].element = ZOO_E_SOLID;
	state.board.tiles[10][11].element = ZOO_E_SOLID;
	zoo_element_defs[ZOO_E_CENTIPEDE_HEAD].tick_func(&state, 1);
	capture_centipede_tick_state(2);

	reset_centipede_state(1);
	state.board.stats[1].leader = -1;
	state.board.stats[1].follower = -1;
	state.board.tiles[10][10].element = ZOO_E_CENTIPEDE_SEGMENT;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].color;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x70;
	zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].tick_func(&state, 1);
	capture_centipede_tick_state(3);
	zoo_element_defs[ZOO_E_CENTIPEDE_SEGMENT].tick_func(&state, 1);
	capture_centipede_tick_state(4);
	return 0;
}

uint32_t centipede_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) centipede_tick_outputs;
}

static void capture_star_draws(uint8_t start_color, int16_t phases, int16_t offset) {
	int16_t phase;
	uint8_t ch;
	reset_movement_state("");
	state.board.tiles[10][10].element = ZOO_E_STAR;
	state.board.tiles[10][10].color = start_color;
	for (phase = 0; phase < phases; phase++) {
		state.current_tick = phase;
		ch = 0;
		zoo_element_defs[ZOO_E_STAR].draw_func(&state, 10, 10, &ch);
		element_draw_outputs[offset + (phase * 2)] = ch;
		element_draw_outputs[offset + (phase * 2) + 1] = state.board.tiles[10][10].color;
	}
}

int32_t run_element_draw_capture_checks(void) {
	static const int8_t direction_x[4] = { 0, 0, -1, 1 };
	static const int8_t direction_y[4] = { -1, 1, 0, 0 };
	int16_t index = 0, direction, phase;
	uint8_t ch;

	capture_star_draws(0x0F, 4, 0);
	capture_star_draws(0x0E, 4, 8);
	capture_star_draws(0xFF, 1, 16);
	index = 18;

	for (direction = 0; direction < 4; direction++) {
		reset_movement_state("");
		state.board.tiles[10][10].element = ZOO_E_TRANSPORTER;
		state.board.stats[1].cycle = 2;
		state.board.stats[1].step_x = direction_x[direction];
		state.board.stats[1].step_y = direction_y[direction];
		for (phase = 0; phase < 4; phase++) {
			state.current_tick = phase * 2;
			ch = 0;
			zoo_element_defs[ZOO_E_TRANSPORTER].draw_func(&state, 10, 10, &ch);
			element_draw_outputs[index++] = ch;
		}
	}
	for (direction = 0; direction < 4; direction++) {
		for (phase = 0; phase < 4; phase++) {
			reset_movement_state("");
			state.board.tiles[10][10].element = ZOO_E_TRANSPORTER;
			state.board.tiles[10][10].color = 0x0F;
			state.board.stats[1].cycle = 2;
			state.board.stats[1].step_x = direction_x[direction];
			state.board.stats[1].step_y = direction_y[direction];
			state.current_tick = phase * 2;
			transporter_tick_write_count = 0;
			transporter_tick_write_x = -1;
			transporter_tick_write_y = -1;
			transporter_tick_write_color = -1;
			transporter_tick_write_character = -1;
			video.func_write = test_transporter_tick_write;
			zoo_element_defs[ZOO_E_TRANSPORTER].tick_func(&state, 1);
			element_draw_outputs[index++] = direction;
			element_draw_outputs[index++] = phase;
			element_draw_outputs[index++] = transporter_tick_write_count;
			element_draw_outputs[index++] = transporter_tick_write_x;
			element_draw_outputs[index++] = transporter_tick_write_y;
			element_draw_outputs[index++] = transporter_tick_write_color;
			element_draw_outputs[index++] = transporter_tick_write_character;
		}
	}
	return index == (int16_t) (sizeof(element_draw_outputs) / sizeof(element_draw_outputs[0])) ? 0 : 1;
}

uint32_t element_draw_output_ptr(void) {
	return (uint32_t) (uintptr_t) element_draw_outputs;
}

int32_t run_oop_object_tick_capture_checks(void) {
	static char cycle_source[] = "#CYCLE 7\r#END\r";
	static char walk_source[] = "#WALK EAST\r#IDLE\r";
	static char thud_source[] = "#WALK EAST\r#IDLE\r:THUD\r#CYCLE 9\r#END\r";
	static char ended_source[] = "#END\r";
	static char *const scripts[4] = { cycle_source, walk_source, thud_source, ended_source };
	int16_t case_id, output_index = 0;

	for (case_id = 0; case_id < 4; case_id++) {
		reset_movement_state(scripts[case_id]);
		if (case_id == 2) {
			state.board.tiles[11][10].element = ZOO_E_SOLID;
			state.board.tiles[11][10].color = 0x1F;
		}
		if (case_id == 3) {
			state.board.stats[1].step_x = 1;
			state.board.stats[1].data_pos = -1;
		}
		zoo_element_defs[ZOO_E_OBJECT].tick_func(&state, 1);
		object_tick_outputs[output_index++] = case_id + 1;
		object_tick_outputs[output_index++] = state.board.stats[1].x;
		object_tick_outputs[output_index++] = state.board.stats[1].y;
		object_tick_outputs[output_index++] = state.board.stats[1].cycle;
		object_tick_outputs[output_index++] = state.board.stats[1].step_x;
		object_tick_outputs[output_index++] = state.board.stats[1].step_y;
		object_tick_outputs[output_index++] = state.board.stats[1].data_pos;
		object_tick_outputs[output_index++] = state.board.tiles[10][10].element;
		object_tick_outputs[output_index++] = state.board.tiles[11][10].element;
	}
	return output_index == (int16_t) (sizeof(object_tick_outputs) / sizeof(object_tick_outputs[0])) ? 0 : 1;
}

uint32_t object_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) object_tick_outputs;
}

int32_t run_oop_object_touch_capture_checks(void) {
	static char found_script[] = "#CYCLE 3\r#END\r:TOUCH\r#CYCLE 9\r#END\r";
	static char missing_script[] = "#CYCLE 3\r#END\r";
	static char other_script[] = "#CYCLE 4\r#END\r:TOUCH\r#CYCLE 6\r#END\r";
	int16_t case_id;
	int16_t output_index = 0;

	for (case_id = 0; case_id < 4; case_id++) {
		int16_t target_id = 1;
		int16_t source_id = 0;
		int16_t dx = 6;
		int16_t dy = -4;
		reset_movement_state(case_id == 1 ? missing_script
			: (case_id == 3 ? other_script : found_script));
		state.board.stats[1].cycle = 3;
		state.board.stats[1].data_pos = (case_id == 3) ? 3 : 2;
		if (case_id == 2) {
			state.board.stats[1].p2 = 1;
		}
		if (case_id == 3) {
			static char target_script[] = "#CYCLE 3\r#END\r:TOUCH\r#CYCLE 9\r#END\r";
			state.board.tiles[10][10].element = ZOO_E_EMPTY;
			state.board.tiles[8][8].element = ZOO_E_OBJECT;
			state.board.stats[1].x = 8;
			state.board.stats[1].y = 8;
			state.board.stats[1].cycle = 4;
			state.board.stats[1].data_pos = 3;
			state.board.stat_count = 2;
			zoo_stat_clear(&state.board.stats[2]);
			state.board.stats[2].x = 10;
			state.board.stats[2].y = 10;
			state.board.stats[2].cycle = 3;
			state.board.stats[2].data_pos = 4;
			state.board.stats[2].data = target_script;
			state.board.stats[2].data_len = (int16_t) (sizeof(target_script) - 1);
			target_id = 2;
			source_id = 1;
		}
		zoo_element_defs[ZOO_E_OBJECT].touch_func(&state, 10, 10, source_id, &dx, &dy);
		object_touch_outputs[output_index++] = case_id + 1;
		object_touch_outputs[output_index++] = target_id;
		object_touch_outputs[output_index++] = state.board.stats[1].data_pos;
		object_touch_outputs[output_index++] = state.board.stats[2].data_pos;
		object_touch_outputs[output_index++] = state.board.stats[target_id].cycle;
		object_touch_outputs[output_index++] = state.board.stats[target_id].p2;
		object_touch_outputs[output_index++] = dx;
		object_touch_outputs[output_index++] = dy;
	}
	return output_index == (int16_t) (sizeof(object_touch_outputs) / sizeof(object_touch_outputs[0])) ? 0 : 1;
}

uint32_t object_touch_output_ptr(void) {
	return (uint32_t) (uintptr_t) object_touch_outputs;
}

static void reset_message_timer_state(int16_t ticks, const char *message,
	int16_t current_stat_tick, int16_t timer_x, int16_t timer_y, bool add_later_stat) {
	zoo_stat *timer;
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	memset(message_row_chars, 0, sizeof(message_row_chars));
	memset(message_row_colors, 0, sizeof(message_row_colors));
	video.func_write = test_message_video_write;
	state.d_video = &video;
	zoo_world_create(&state);
	state.tick_duration = 0;
	zoo_display_message(&state, ticks, (char *) message);
	timer = &state.board.stats[1];
	if (timer_x != 0 || timer_y != 0) {
		state.board.tiles[timer->x][timer->y] = timer->under;
		timer->x = timer_x;
		timer->y = timer_y;
		timer->under = state.board.tiles[timer_x][timer_y];
		state.board.tiles[timer_x][timer_y].element = ZOO_E_MESSAGE_TIMER;
		state.board.tiles[timer_x][timer_y].color = 0;
	}
	if (add_later_stat) {
		zoo_stat_add(&state, 20, 10, ZOO_E_OBJECT, zoo_element_defs[ZOO_E_OBJECT].color,
			1, &zoo_stat_template_default);
	}
	state.current_stat_tick = current_stat_tick;
}

static void capture_message_timer_state(int16_t case_id, int16_t timer_x, int16_t timer_y) {
	int16_t *out = &message_timer_outputs[(case_id - 1) * 138];
	int16_t timer_stat = zoo_stat_get_id(&state, timer_x, timer_y);
	int16_t i;
	int16_t message_len = 0;
	*out++ = state.board.stat_count;
	*out++ = state.current_stat_tick;
	while (message_len < ZOO_LEN_MESSAGE && state.board.info.message[message_len] != '\0') {
		message_len++;
	}
	*out++ = message_len;
	*out++ = timer_stat > 0 ? state.board.stats[timer_stat].p2 : -1;
	*out++ = state.board.tiles[0][0].element;
	*out++ = state.board.tiles[0][0].color;
	*out++ = state.board.tiles[1][0].element;
	*out++ = state.board.tiles[1][0].color;
	for (i = 0; i < 10; i++) {
		*out++ = i < message_len ? (uint8_t) state.board.info.message[i] : -1;
	}
	for (i = 0; i < 60; i++) {
		*out++ = message_row_chars[i];
		*out++ = message_row_colors[i];
	}
}

int32_t run_message_timer_capture_checks(void) {
	reset_message_timer_state(3, "AB", 1, 0, 0, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(1, 0, 0);

	reset_message_timer_state(1, "LAST", 1, 0, 0, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(2, 0, 0);

	reset_message_timer_state(1, "MID", 2, 0, 0, true);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(3, 0, 0);

	reset_message_timer_state(2, "HOLD", 1, 1, 0, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(4, 1, 0);

	reset_message_timer_state(-1, "WRAP", 1, 0, 0, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(5, 0, 0);

	reset_message_timer_state(2, "SIDE", 1, 0, 1, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(6, 0, 1);

	reset_message_timer_state(0, "ZERO", 1, 0, 0, false);
	zoo_element_defs[ZOO_E_MESSAGE_TIMER].tick_func(&state, 1);
	capture_message_timer_state(7, 0, 0);
	return 0;
}

uint32_t message_timer_output_ptr(void) {
	return (uint32_t) (uintptr_t) message_timer_outputs;
}

static void reset_blink_wall_tick_state(bool right_is_empty) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.tick_duration = 8;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.board.stat_count = 1;
	state.board.stats[0].x = 20;
	state.board.stats[0].y = 11;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0x70;
	state.board.tiles[20][11].element = ZOO_E_PLAYER;
	state.board.tiles[20][11].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = 20;
	state.board.stats[1].y = 10;
	state.board.stats[1].step_x = 0;
	state.board.stats[1].step_y = 1;
	state.board.stats[1].p1 = 0;
	state.board.stats[1].p2 = 1;
	state.board.stats[1].p3 = 0;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[20][10].element = ZOO_E_BLINK_WALL;
	state.board.tiles[20][10].color = 7;
	state.board.tiles[21][11].element = right_is_empty ? ZOO_E_EMPTY : ZOO_E_SOLID;
	state.board.tiles[21][11].color = 0x70;
	state.board.tiles[19][11].element = right_is_empty ? ZOO_E_SOLID : ZOO_E_EMPTY;
	state.board.tiles[19][11].color = 0x70;
	state.board.tiles[20][15].element = ZOO_E_SOLID;
	state.board.tiles[20][15].color = 0x70;
	state.current_stat_tick = 1;
	video_write_count = 0;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
}

static void capture_blink_wall_tick_state(int16_t case_id) {
	int16_t *out = &blink_wall_tick_outputs[(case_id - 1) * 21];
	int16_t timer_stat = zoo_stat_get_id(&state, 0, 0);
	int16_t message_len = 0;
	int16_t i;
	while (message_len < ZOO_LEN_MESSAGE && state.board.info.message[message_len] != '\0') {
		message_len++;
	}
	*out++ = case_id;
	*out++ = state.board.stats[0].x;
	*out++ = state.board.stats[0].y;
	*out++ = state.world.info.health;
	*out++ = state.board.stat_count;
	*out++ = timer_stat > 0 ? state.board.stats[timer_stat].p2 : -1;
	*out++ = message_len;
	for (i = 0; i < 5; i++) {
		*out++ = i < message_len ? (uint8_t) state.board.info.message[i] : -1;
	}
	*out++ = state.board.stats[0].under.element;
	*out++ = state.board.stats[0].under.color;
	*out++ = state.board.tiles[20][11].element;
	*out++ = state.board.tiles[20][11].color;
	*out++ = state.board.tiles[21][11].element;
	*out++ = state.board.tiles[21][11].color;
	*out++ = state.board.tiles[19][11].element;
	*out++ = state.board.tiles[19][11].color;
	*out++ = state.board.stats[1].p3;
}

int32_t run_blink_wall_tick_capture_checks(void) {
	reset_blink_wall_tick_state(true);
	zoo_element_defs[ZOO_E_BLINK_WALL].tick_func(&state, 1);
	capture_blink_wall_tick_state(1);

	reset_blink_wall_tick_state(false);
	zoo_element_defs[ZOO_E_BLINK_WALL].tick_func(&state, 1);
	capture_blink_wall_tick_state(2);
	return 0;
}

uint32_t blink_wall_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) blink_wall_tick_outputs;
}

int32_t run_scroll_tick_capture_checks(void) {
	int16_t color;
	int16_t *out = scroll_tick_outputs;

	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.board.stat_count = 1;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.tiles[10][10].element = ZOO_E_SCROLL;
	state.current_stat_tick = 1;

	for (color = 0; color < 256; color++) {
		state.board.tiles[10][10].color = (uint8_t) color;
		zoo_element_defs[ZOO_E_SCROLL].tick_func(&state, 1);
		*out++ = color;
		*out++ = state.board.tiles[10][10].color;
	}
	return 0;
}

uint32_t scroll_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) scroll_tick_outputs;
}

int32_t run_scroll_touch_capture_checks(void) {
	static const char *scripts[2] = {
		"First line\rSecond line\r",
		"@Scroll custom title\rFirst line\rSecond line\r"
	};
	static const char *titles[2] = { "Scroll", "Scroll custom title" };
	int16_t dx = 0;
	int16_t dy = 0;
	int case_index;

	for (case_index = 0; case_index < 2; case_index++) {
		int16_t *out = scroll_touch_outputs + (case_index * 155);
		int title_length;
		int i;
		reset_movement_state(scripts[case_index]);
		state.board.tiles[10][10].element = ZOO_E_SCROLL;
		memset(scroll_touch_lines, 0, sizeof(scroll_touch_lines));
		scroll_touch_line_count = 0;
		capture_scroll_touch_lines = true;
		zoo_element_defs[ZOO_E_SCROLL].touch_func(&state, 10, 10, 0, &dx, &dy);
		capture_scroll_touch_lines = false;
		if (state.object_window.line_count != 2) return 1 + case_index;
		if (strcmp(state.object_window.title, titles[case_index]) != 0) return 3 + case_index;
		if (scroll_touch_line_count != 2
			|| strcmp(scroll_touch_lines[0], "First line") != 0
			|| strcmp(scroll_touch_lines[1], "Second line") != 0) return 5 + case_index;
		title_length = (int) strlen(state.object_window.title);
		*out++ = state.object_window.line_count;
		*out++ = title_length;
		for (i = 0; i < 50; i++)
			*out++ = (i < title_length) ? (uint8_t) state.object_window.title[i] : 0;
		*out++ = scroll_touch_line_count;
		for (i = 0; i < 2; i++) {
			int line_length = (int) strlen(scroll_touch_lines[i]);
			int j;
			*out++ = line_length;
			for (j = 0; j < 50; j++)
				*out++ = (j < line_length) ? (uint8_t) scroll_touch_lines[i][j] : 0;
		}
	}
	return 0;
}

uint32_t scroll_touch_output_ptr(void) {
	return (uint32_t) (uintptr_t) scroll_touch_outputs;
}

static void reset_bear_state(int16_t player_x, int16_t player_y, int16_t bear_x,
	int16_t bear_y, int16_t p1) {
	reset_movement_state("");
	state.board.stats[0].x = (uint8_t) player_x;
	state.board.stats[0].y = (uint8_t) player_y;
	state.board.tiles[player_x][player_y].element = ZOO_E_PLAYER;
	state.board.tiles[player_x][player_y].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = (uint8_t) bear_x;
	state.board.stats[1].y = (uint8_t) bear_y;
	state.board.stats[1].p1 = p1;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[bear_x][bear_y].element = ZOO_E_BEAR;
	state.board.tiles[bear_x][bear_y].color = zoo_element_defs[ZOO_E_BEAR].color;
}

static int check_bear_chase(void) {
	reset_bear_state(15, 13, 10, 10, 0);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].element != ZOO_E_BEAR) return 2;

	reset_bear_state(10, 13, 10, 10, 0);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 11) return 3;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][11].element != ZOO_E_BEAR) return 4;

	reset_bear_state(15, 13, 10, 10, 6);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 10) return 5;
	if (state.board.tiles[10][10].element != ZOO_E_BEAR) return 6;
	return 0;
}

static void capture_bear_tick_state(int16_t case_id) {
	int16_t *out = &bear_tick_outputs[(case_id - 1) * 5];
	*out++ = case_id;
	*out++ = state.board.stats[1].x;
	*out++ = state.board.stats[1].y;
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[10][10].color;
}

int32_t run_bear_tick_capture_checks(void) {
	reset_bear_state(15, 13, 10, 10, 0);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	capture_bear_tick_state(1);

	reset_bear_state(10, 13, 10, 10, 0);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	capture_bear_tick_state(2);

	reset_bear_state(15, 13, 10, 10, 6);
	zoo_element_defs[ZOO_E_BEAR].tick_func(&state, 1);
	capture_bear_tick_state(3);
	return 0;
}

uint32_t bear_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) bear_tick_outputs;
}

static void reset_shark_state(int16_t player_x, int16_t player_y, int16_t shark_x,
	int16_t shark_y) {
	reset_movement_state("");
	state.func_random = test_zero_random;
	state.board.stats[0].x = (uint8_t) player_x;
	state.board.stats[0].y = (uint8_t) player_y;
	state.board.tiles[player_x][player_y].element = ZOO_E_PLAYER;
	state.board.tiles[player_x][player_y].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = (uint8_t) shark_x;
	state.board.stats[1].y = (uint8_t) shark_y;
	state.board.stats[1].p1 = 0;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[shark_x][shark_y].element = ZOO_E_SHARK;
	state.board.tiles[shark_x][shark_y].color = zoo_element_defs[ZOO_E_SHARK].color;
}

static int check_shark_water_only_movement(void) {
	reset_shark_state(13, 10, 10, 10);
	state.board.tiles[11][10].element = ZOO_E_WATER;
	state.board.tiles[11][10].color = 0x13;
	zoo_element_defs[ZOO_E_SHARK].tick_func(&state, 1);
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].element != ZOO_E_SHARK) return 2;
	if (state.board.stats[1].under.element != ZOO_E_WATER
		|| state.board.stats[1].under.color != 0x13) return 3;

	reset_shark_state(13, 10, 10, 10);
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x31;
	zoo_element_defs[ZOO_E_SHARK].tick_func(&state, 1);
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 10) return 4;
	if (state.board.tiles[10][10].element != ZOO_E_SHARK
		|| state.board.tiles[11][10].element != ZOO_E_EMPTY) return 5;
	return 0;
}

static void capture_shark_tick_state(int16_t case_id) {
	int16_t *out = &shark_tick_outputs[(case_id - 1) * 7];
	*out++ = case_id;
	*out++ = state.board.stats[1].x;
	*out++ = state.board.stats[1].y;
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[11][10].element;
	*out++ = state.board.stats[1].under.element;
	*out++ = state.board.stats[1].under.color;
}

int32_t run_shark_tick_capture_checks(void) {
	reset_shark_state(13, 10, 10, 10);
	state.board.tiles[11][10].element = ZOO_E_WATER;
	state.board.tiles[11][10].color = 0x13;
	zoo_element_defs[ZOO_E_SHARK].tick_func(&state, 1);
	capture_shark_tick_state(1);

	reset_shark_state(13, 10, 10, 10);
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x31;
	zoo_element_defs[ZOO_E_SHARK].tick_func(&state, 1);
	capture_shark_tick_state(2);
	return 0;
}

uint32_t shark_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) shark_tick_outputs;
}

static void reset_lion_state(int16_t player_x, int16_t player_y, int16_t lion_x,
	int16_t lion_y, int16_t p1) {
	reset_movement_state("");
	state.func_random = test_creature_random;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	state.board.stats[0].x = (uint8_t) player_x;
	state.board.stats[0].y = (uint8_t) player_y;
	state.board.tiles[player_x][player_y].element = ZOO_E_PLAYER;
	state.board.tiles[player_x][player_y].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = (uint8_t) lion_x;
	state.board.stats[1].y = (uint8_t) lion_y;
	state.board.stats[1].p1 = p1;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[lion_x][lion_y].element = ZOO_E_LION;
	state.board.tiles[lion_x][lion_y].color = zoo_element_defs[ZOO_E_LION].color;
}

static int check_lion_direction_branches(void) {
	reset_lion_state(13, 11, 10, 10, 0);
	creature_random_value[0] = 9;
	creature_random_value[1] = 2;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	if (creature_random_count != 2 || creature_random_max[0] != 10
		|| creature_random_max[1] != 3) return 1;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 2;

	reset_lion_state(13, 13, 10, 10, 0);
	creature_random_value[0] = 9;
	creature_random_value[1] = 1;
	creature_random_value[2] = 1;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	if (creature_random_count != 3 || creature_random_max[0] != 10
		|| creature_random_max[1] != 3 || creature_random_max[2] != 2) return 3;
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 11) return 4;

	reset_lion_state(13, 10, 10, 10, 0);
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	if (creature_random_count != 2 || creature_random_max[0] != 10
		|| creature_random_max[1] != 2) return 5;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 6;
	return 0;
}

static void capture_lion_tick_state(int16_t case_id) {
	int16_t *out = &lion_tick_outputs[(case_id - 1) * 12];
	int16_t i;
	*out++ = case_id;
	*out++ = creature_random_count;
	for (i = 0; i < 3; i++) {
		*out++ = i < creature_random_count ? creature_random_value[i] : -1;
	}
	for (i = 0; i < 3; i++) {
		*out++ = i < creature_random_count ? creature_random_max[i] : -1;
	}
	*out++ = state.board.stats[1].x;
	*out++ = state.board.stats[1].y;
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.stats[1].under.element;
}

int32_t run_lion_tick_capture_checks(void) {
	reset_lion_state(13, 11, 10, 10, 0);
	creature_random_value[0] = 9;
	creature_random_value[1] = 2;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	capture_lion_tick_state(1);

	reset_lion_state(13, 13, 10, 10, 0);
	creature_random_value[0] = 9;
	creature_random_value[1] = 1;
	creature_random_value[2] = 1;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	capture_lion_tick_state(2);

	reset_lion_state(13, 10, 10, 10, 0);
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	zoo_element_defs[ZOO_E_LION].tick_func(&state, 1);
	capture_lion_tick_state(3);
	return 0;
}

uint32_t lion_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) lion_tick_outputs;
}

static void reset_ruffian_state(void) {
	reset_movement_state("");
	state.func_random = test_creature_random;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	state.board.stats[0].x = 20;
	state.board.stats[0].y = 10;
	state.board.tiles[20][10].element = ZOO_E_PLAYER;
	state.board.tiles[20][10].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].x = 10;
	state.board.stats[1].y = 10;
	state.board.stats[1].p1 = 8;
	state.board.stats[1].p2 = 8;
	state.board.stats[1].step_x = 0;
	state.board.stats[1].step_y = 0;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_RUFFIAN;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_RUFFIAN].color;
	state.board.tiles[11][10].color = 0;
	state.board.tiles[12][10].element = ZOO_E_EMPTY;
	state.board.tiles[12][10].color = 0;
}

static int check_ruffian_wake_and_stop(void) {
	reset_ruffian_state();
	creature_random_value[0] = 16;
	creature_random_value[1] = 8;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	if (creature_random_count != 3 || creature_random_max[0] != 17
		|| creature_random_max[1] != 9 || creature_random_max[2] != 2) return 1;
	if (state.board.stats[1].x != 10 || state.board.stats[1].y != 10
		|| state.board.stats[1].step_x != 1 || state.board.stats[1].step_y != 0) return 2;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0x70
		|| state.board.tiles[10][10].element != ZOO_E_RUFFIAN
		|| state.board.tiles[10][10].color != zoo_element_defs[ZOO_E_RUFFIAN].color
		|| state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0) return 7;

	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	if (creature_random_count != 3 || creature_random_max[0] != 9
		|| creature_random_max[1] != 2 || creature_random_max[2] != 17) return 3;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10
		|| state.board.stats[1].step_x != 1 || state.board.stats[1].step_y != 0) return 4;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0
		|| state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70
		|| state.board.tiles[11][10].element != ZOO_E_RUFFIAN
		|| state.board.tiles[11][10].color != zoo_element_defs[ZOO_E_RUFFIAN].color
		|| state.board.tiles[12][10].element != ZOO_E_EMPTY
		|| state.board.tiles[12][10].color != 0) return 8;

	state.board.stats[1].p1 = 0;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	creature_random_value[0] = 8;
	creature_random_value[1] = 16;
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	if (creature_random_count != 2 || creature_random_max[0] != 9
		|| creature_random_max[1] != 17) return 5;
	if (state.board.stats[1].x != 12 || state.board.stats[1].y != 10
		|| state.board.stats[1].step_x != 0 || state.board.stats[1].step_y != 0) return 6;
	if (state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0
		|| state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0
		|| state.board.tiles[12][10].element != ZOO_E_RUFFIAN
		|| state.board.tiles[12][10].color != zoo_element_defs[ZOO_E_RUFFIAN].color) return 9;
	return 0;
}

static void capture_ruffian_tick_state(int16_t case_id) {
	int16_t *out = &ruffian_tick_outputs[(case_id - 1) * 22];
	int16_t i;
	*out++ = case_id;
	*out++ = creature_random_count;
	for (i = 0; i < 3; i++) {
		*out++ = i < creature_random_count ? creature_random_value[i] : -1;
	}
	for (i = 0; i < 3; i++) {
		*out++ = i < creature_random_count ? creature_random_max[i] : -1;
	}
	*out++ = state.board.stats[0].x;
	*out++ = state.board.stats[0].y;
	*out++ = state.board.stats[1].x;
	*out++ = state.board.stats[1].y;
	*out++ = state.board.stats[1].step_x;
	*out++ = state.board.stats[1].step_y;
	*out++ = state.board.stats[1].under.element;
	*out++ = state.board.stats[1].under.color;
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[10][10].color;
	*out++ = state.board.tiles[11][10].element;
	*out++ = state.board.tiles[11][10].color;
	*out++ = state.board.tiles[12][10].element;
	*out++ = state.board.tiles[12][10].color;
}

int32_t run_ruffian_tick_capture_checks(void) {
	reset_ruffian_state();
	creature_random_value[0] = 16;
	creature_random_value[1] = 8;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	capture_ruffian_tick_state(1);

	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	capture_ruffian_tick_state(2);

	state.board.stats[1].p1 = 0;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	creature_random_value[0] = 8;
	creature_random_value[1] = 16;
	zoo_element_defs[ZOO_E_RUFFIAN].tick_func(&state, 1);
	capture_ruffian_tick_state(3);
	return 0;
}

uint32_t ruffian_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) ruffian_tick_outputs;
}

static void reset_tiger_state(int16_t player_x, int16_t player_y, int16_t p2) {
	reset_lion_state(player_x, player_y, 10, 10, 0);
	state.board.stats[1].p2 = p2;
	state.board.tiles[10][10].element = ZOO_E_TIGER;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_TIGER].color;
}

static int check_tiger_shot_priority(void) {
	reset_tiger_state(11, 13, 0);
	state.board.tiles[10][11].element = ZOO_E_SOLID;
	state.board.tiles[10][11].color = zoo_element_defs[ZOO_E_SOLID].color;
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_TIGER].tick_func(&state, 1);
	if (creature_random_count != 3 || creature_random_max[0] != 10
		|| creature_random_max[1] != 10 || creature_random_max[2] != 2) return 1;
	if (state.board.stat_count != 1 || state.board.stats[1].x != 11
		|| state.board.stats[1].y != 10) return 2;
	if (state.board.tiles[10][11].element != ZOO_E_SOLID
		|| state.board.tiles[11][10].element != ZOO_E_TIGER) return 3;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70
		|| state.board.tiles[10][11].color != zoo_element_defs[ZOO_E_SOLID].color
		|| state.board.tiles[11][10].color != zoo_element_defs[ZOO_E_TIGER].color) return 7;

	reset_tiger_state(13, 10, 0x80);
	state.board.tiles[10][11].element = ZOO_E_EMPTY;
	state.board.tiles[10][11].color = 0;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0;
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_TIGER].tick_func(&state, 1);
	if (creature_random_count != 3 || creature_random_max[0] != 10
		|| creature_random_max[1] != 10 || creature_random_max[2] != 2) return 4;
	if (state.board.stat_count != 2) return 5;
	if (state.board.tiles[11][10].element != ZOO_E_STAR
		|| state.board.stats[2].p1 != 1 || state.board.stats[2].step_x != 1
		|| state.board.stats[2].step_y != 0 || state.board.stats[2].p2 != 100) return 6;
	if (state.board.tiles[10][10].element != ZOO_E_TIGER
		|| state.board.tiles[10][10].color != zoo_element_defs[ZOO_E_TIGER].color
		|| state.board.tiles[10][11].element != ZOO_E_EMPTY
		|| state.board.tiles[10][11].color != 0
		|| state.board.tiles[11][10].color != 9) return 8;
	return 0;
}

static void capture_tiger_tick_state(int16_t case_id) {
	static const int16_t tile_x[3] = { 10, 10, 11 };
	static const int16_t tile_y[3] = { 10, 11, 10 };
	int16_t *out = &tiger_tick_outputs[(case_id - 1) * 23];
	int16_t i;
	*out++ = case_id;
	*out++ = creature_random_count;
	for (i = 0; i < 3; i++) *out++ = creature_random_value[i];
	for (i = 0; i < 3; i++) *out++ = creature_random_max[i];
	*out++ = state.board.stat_count;
	if (state.board.stat_count > 0) {
		*out++ = state.board.stats[1].x;
		*out++ = state.board.stats[1].y;
	} else {
		*out++ = -1;
		*out++ = -1;
	}
	for (i = 0; i < 3; i++) {
		*out++ = state.board.tiles[tile_x[i]][tile_y[i]].element;
		*out++ = state.board.tiles[tile_x[i]][tile_y[i]].color;
	}
	if (state.board.stat_count > 1) {
		*out++ = state.board.stats[2].x;
		*out++ = state.board.stats[2].y;
		*out++ = state.board.stats[2].p1;
		*out++ = state.board.stats[2].step_x;
		*out++ = state.board.stats[2].step_y;
		*out++ = state.board.stats[2].p2;
	} else {
		for (i = 0; i < 6; i++) *out++ = -1;
	}
}

int32_t run_tiger_tick_capture_checks(void) {
	reset_tiger_state(11, 13, 0);
	state.board.tiles[10][11].element = ZOO_E_SOLID;
	state.board.tiles[10][11].color = zoo_element_defs[ZOO_E_SOLID].color;
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_TIGER].tick_func(&state, 1);
	capture_tiger_tick_state(1);

	reset_tiger_state(13, 10, 0x80);
	state.board.tiles[10][11].element = ZOO_E_EMPTY;
	state.board.tiles[10][11].color = 0;
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0;
	creature_random_value[0] = 0;
	creature_random_value[1] = 0;
	creature_random_value[2] = 0;
	zoo_element_defs[ZOO_E_TIGER].tick_func(&state, 1);
	capture_tiger_tick_state(2);
	return 0;
}

uint32_t tiger_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) tiger_tick_outputs;
}

static int check_star_tick_period_and_water(void) {
	reset_movement_state("");
	state.func_random = test_creature_random;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	state.board.stats[0].x = 13;
	state.board.stats[0].y = 10;
	state.board.tiles[13][10].element = ZOO_E_PLAYER;
	state.board.tiles[13][10].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].p2 = 4;
	state.board.tiles[10][10].element = ZOO_E_STAR;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_STAR].color;
	state.board.tiles[11][10].element = ZOO_E_WATER;
	state.board.tiles[11][10].color = 0x13;

	zoo_element_defs[ZOO_E_STAR].tick_func(&state, 1);
	if (state.board.stats[1].p2 != 3 || state.board.stats[1].x != 10
		|| state.board.stats[1].y != 10 || creature_random_count != 0) return 1;
	if (state.board.tiles[10][10].element != ZOO_E_STAR
		|| state.board.tiles[10][10].color != 9) return 9;

	zoo_element_defs[ZOO_E_STAR].tick_func(&state, 1);
	if (state.board.stats[1].p2 != 2) return 2;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10) return 3;
	if (creature_random_count != 1) return 4;
	if (creature_random_max[0] != 2) return 5;
	if (state.board.stats[1].under.element != ZOO_E_WATER
		|| state.board.stats[1].under.color != 0x13) return 6;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70
		|| state.board.tiles[11][10].element != ZOO_E_STAR
		|| state.board.tiles[11][10].color != 9) return 10;

	zoo_element_defs[ZOO_E_STAR].tick_func(&state, 1);
	if (state.board.stats[1].p2 != 1 || state.board.stats[1].x != 11
		|| state.board.stats[1].y != 10 || creature_random_count != 1) return 7;
	if (state.board.tiles[11][10].element != ZOO_E_STAR
		|| state.board.tiles[11][10].color != 10) return 11;

	zoo_element_defs[ZOO_E_STAR].tick_func(&state, 1);
	if (state.board.stat_count != 0 || state.board.tiles[11][10].element != ZOO_E_WATER
		|| state.board.tiles[11][10].color != 0x13) return 8;
	if (state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x70) return 12;
	return 0;
}

static void capture_star_tick_state(int16_t case_id) {
	int16_t *out = &star_tick_outputs[(case_id - 1) * 13];
	*out++ = case_id;
	if (state.board.stat_count > 0) {
		*out++ = state.board.stats[1].p2;
		*out++ = state.board.stats[1].x;
		*out++ = state.board.stats[1].y;
		*out++ = state.board.stats[1].step_x;
		*out++ = state.board.stats[1].step_y;
		*out++ = state.board.stats[1].under.element;
		*out++ = state.board.stats[1].under.color;
	} else {
		int16_t i;
		for (i = 0; i < 7; i++) *out++ = -1;
	}
	*out++ = state.board.tiles[10][10].element;
	*out++ = state.board.tiles[10][10].color;
	*out++ = state.board.tiles[11][10].element;
	*out++ = state.board.tiles[11][10].color;
	*out++ = state.board.stat_count;
}

int32_t run_star_tick_capture_checks(void) {
	int16_t i;
	reset_movement_state("");
	state.func_random = test_creature_random;
	creature_random_count = 0;
	memset(creature_random_max, 0, sizeof(creature_random_max));
	memset(creature_random_value, 0, sizeof(creature_random_value));
	state.board.stats[0].x = 13;
	state.board.stats[0].y = 10;
	state.board.tiles[13][10].element = ZOO_E_PLAYER;
	state.board.tiles[13][10].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.stats[1].p2 = 4;
	state.board.tiles[10][10].element = ZOO_E_STAR;
	state.board.tiles[10][10].color = zoo_element_defs[ZOO_E_STAR].color;
	state.board.tiles[11][10].element = ZOO_E_WATER;
	state.board.tiles[11][10].color = 0x13;
	state.world.info.energizer_ticks = 0;

	for (i = 1; i <= 4; i++) {
		zoo_element_defs[ZOO_E_STAR].tick_func(&state, 1);
		capture_star_tick_state(i);
	}
	return 0;
}

uint32_t star_tick_output_ptr(void) {
	return (uint32_t) (uintptr_t) star_tick_outputs;
}

static int check_energizer_touch(void) {
	static char object_script[] = "@TARGET\r:ENERGIZE\r#CYCLE 5\r";
	static const char expected_message[] = "Energizer - You are invincible";
	int16_t dx = 3;
	int16_t dy = -2;
	int i;
	reset_movement_state(object_script);
	state.board.stats[0].x = 3;
	state.board.stats[0].y = 3;
	state.board.tiles[3][3].element = ZOO_E_PLAYER;
	state.board.stats[1].data_pos = 0;
	state.board.stats[1].p2 = 0;
	state.board.stats[1].data_len = (int16_t) strlen(object_script);
	state.board.tiles[11][10].element = ZOO_E_ENERGIZER;
	state.board.tiles[11][10].color = 0x3F;
	state.world.info.energizer_ticks = 0;
	state.tick_duration = 8;
	state.msg_flags.energizer = true;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;

	zoo_element_defs[ZOO_E_ENERGIZER].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x3F) return 1;
	if (state.world.info.energizer_ticks != 75) return 2;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_ALL) return 3;
	if (state.sound.current_priority != 9 || state.sound.buffer_len != 112
		|| state.sound.buffer[0] != 0x20 || state.sound.buffer[1] != 0x03
		|| state.sound.buffer[110] != 0x20 || state.sound.buffer[111] != 0x03) return 4;
	if (state.board.stat_count != 2) return 5;
	if (state.board.info.io_message_length != sizeof(expected_message) - 1)
		return 100 + state.board.info.io_message_length;
	for (i = 0; i < (int) sizeof(expected_message) - 1; i++) {
		if (state.board.info.message[i] != expected_message[i]) return 30 + i;
	}
	if (state.board.info.message[sizeof(expected_message) - 1] != '\0') return 60;
	if (state.msg_flags.energizer) return 7;
	if (state.board.stats[2].x != 0 || state.board.stats[2].y != 0
		|| state.board.stats[2].p2 != 22) return 8;
	if (state.board.stats[1].data_pos != 7) return 9;
	if (dx != 3 || dy != -2) return 10;
	return 0;
}

static int check_torch_touch(void) {
	static const char expected_message[] = "Torch - used for lighting in the underground.";
	static const uint8_t expected_sound[] = { 0x30, 0x01, 0x39, 0x01, 0x34, 0x02 };
	int16_t dx = 2;
	int16_t dy = -1;
	int i;
	reset_movement_state("");
	state.board.stat_count = 0;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = 0x1F;
	state.board.tiles[11][10].element = ZOO_E_TORCH;
	state.board.tiles[11][10].color = 0x2E;
	state.world.info.torches = INT16_MAX;
	state.game_state = GS_PLAY;
	state.tick_duration = 4;
	state.msg_flags.torch = true;
	state.sound.current_priority = -1;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;

	zoo_element_defs[ZOO_E_TORCH].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.torches != INT16_MIN) return 1;
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2E) return 2;
	if (state.board.info.io_message_length != sizeof(expected_message) - 1)
		return 100 + state.board.info.io_message_length;
	for (i = 0; i < (int) sizeof(expected_message) - 1; i++) {
		if (state.board.info.message[i] != expected_message[i]) return 30 + i;
	}
	if (state.board.info.message[sizeof(expected_message) - 1] != '\0') return 70;
	if (state.msg_flags.torch) return 3;
	if (state.board.stat_count != 1 || state.board.stats[1].x != 0
		|| state.board.stats[1].y != 0 || state.board.stats[1].p2 != 40) return 4;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_TORCHES) return 5;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(expected_sound)) return 6;
	for (i = 0; i < (int) sizeof(expected_sound); i++) {
		if (state.sound.buffer[i] != expected_sound[i]) return 10 + i;
	}
	if (dx != 2 || dy != -1) return 20;

	reset_movement_state("");
	state.board.stat_count = 0;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[11][10].element = ZOO_E_TORCH;
	state.board.tiles[11][10].color = 0x2E;
	state.world.info.torches = 0;
	state.game_state = GS_PLAY;
	state.tick_duration = 4;
	state.msg_flags.torch = false;
	state.sound.current_priority = -1;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	dx = 2;
	dy = -1;

	zoo_element_defs[ZOO_E_TORCH].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.torches != 1 || state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2E) return 30;
	if (state.board.info.io_message_length != 0 || state.board.stat_count != 0
		|| state.msg_flags.torch) return 31;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_TORCHES) return 32;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(expected_sound)) return 33;
	for (i = 0; i < (int) sizeof(expected_sound); i++) {
		if (state.sound.buffer[i] != expected_sound[i]) return 40 + i;
	}
	if (dx != 2 || dy != -1) return 50;
	return 0;
}

static void reset_terrain_touch_scene(int element, uint8_t color) {
	reset_movement_state("");
	state.board.stat_count = 0;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[11][10].element = (uint8_t) element;
	state.board.tiles[11][10].color = color;
	state.tick_duration = 4;
	state.sound.current_priority = -1;
}

static int check_terrain_touches(void) {
	static const char invisible_message[] = "You are blocked by an invisible wall.";
	static const char forest_message[] = "A path is cleared through the forest.";
	static const char fake_message[] = "A fake wall - secret passage!";
	static const char water_message[] = "Your way is blocked by water.";
	static const uint8_t invisible_sound[] = { 18, 1, 16, 1 };
	static const uint8_t forest_sound[] = { 57, 1 };
	static const uint8_t water_sound[] = { 64, 1, 80, 1 };
	int16_t dx = 2;
	int16_t dy = -1;
	int i;

	reset_terrain_touch_scene(ZOO_E_INVISIBLE, 0x2A);
	zoo_element_defs[ZOO_E_INVISIBLE].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_NORMAL
		|| state.board.tiles[11][10].color != 0x2A) return 1;
	if (strcmp(state.board.info.message, invisible_message) != 0
		|| state.board.info.io_message_length != sizeof(invisible_message) - 1
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 20) return 2;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(invisible_sound)) return 3;
	for (i = 0; i < (int) sizeof(invisible_sound); i++) {
		if (state.sound.buffer[i] != invisible_sound[i]) return 4 + i;
	}
	if (dx != 2 || dy != -1) return 8;

	dx = 2;
	dy = -1;
	reset_terrain_touch_scene(ZOO_E_FOREST, 0x2B);
	state.msg_flags.forest = true;
	zoo_element_defs[ZOO_E_FOREST].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2B || state.msg_flags.forest) return 10;
	if (strcmp(state.board.info.message, forest_message) != 0
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 40) return 11;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(forest_sound)
		|| state.sound.buffer[0] != forest_sound[0] || state.sound.buffer[1] != forest_sound[1]) return 12;
	if (dx != 2 || dy != -1) return 13;

	dx = 2;
	dy = -1;
	reset_terrain_touch_scene(ZOO_E_FOREST, 0x2B);
	state.msg_flags.forest = false;
	zoo_element_defs[ZOO_E_FOREST].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2B || state.msg_flags.forest) return 14;
	if (state.board.info.io_message_length != 0 || state.board.stat_count != 0) return 15;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(forest_sound)
		|| state.sound.buffer[0] != forest_sound[0] || state.sound.buffer[1] != forest_sound[1]) return 16;
	if (dx != 2 || dy != -1) return 17;

	dx = 2;
	dy = -1;
	reset_terrain_touch_scene(ZOO_E_FAKE, 0x2C);
	state.msg_flags.fake = true;
	zoo_element_defs[ZOO_E_FAKE].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_FAKE
		|| state.board.tiles[11][10].color != 0x2C || state.msg_flags.fake) return 18;
	if (strcmp(state.board.info.message, fake_message) != 0
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 30) return 19;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 20;
	if (dx != 2 || dy != -1) return 21;

	dx = 2;
	dy = -1;
	reset_terrain_touch_scene(ZOO_E_FAKE, 0x2C);
	state.msg_flags.fake = false;
	zoo_element_defs[ZOO_E_FAKE].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_FAKE
		|| state.board.tiles[11][10].color != 0x2C || state.msg_flags.fake) return 22;
	if (state.board.info.io_message_length != 0 || state.board.stat_count != 0
		|| state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 23;
	if (dx != 2 || dy != -1) return 24;

	dx = 2;
	dy = -1;
	reset_terrain_touch_scene(ZOO_E_WATER, 0x2D);
	zoo_element_defs[ZOO_E_WATER].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.tiles[11][10].element != ZOO_E_WATER
		|| state.board.tiles[11][10].color != 0x2D) return 25;
	if (strcmp(state.board.info.message, water_message) != 0
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 20) return 26;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(water_sound)) return 27;
	for (i = 0; i < (int) sizeof(water_sound); i++) {
		if (state.sound.buffer[i] != water_sound[i]) return 28 + i;
	}
	if (dx != 2 || dy != -1) return 32;
	return 0;
}

static int check_shared_damage_and_slime_touch(void) {
	static const char expected_message[] = "Ouch!";
	static const uint8_t nonlethal_sound[] = { 16, 1, 32, 1, 19, 1, 35, 1 };
	static const uint8_t lethal_sound[] = {
		32, 3, 35, 3, 39, 3, 48, 3, 39, 3, 42, 3, 50, 3,
		55, 3, 53, 3, 56, 3, 64, 3, 69, 3, 16, 10
	};
	static const uint8_t slime_sound[] = { 64, 1, 16, 1, 80, 1, 48, 1 };
	int16_t dx = 2;
	int16_t dy = -1;
	int i;

	reset_movement_state("");
	state.board.stat_count = 0;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = 0x1F;
	state.world.info.health = 15;
	state.game_state = GS_PLAY;
	state.tick_duration = 4;
	state.sound.current_priority = -1;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_board_damage_stat(&state, 0);
	if (state.world.info.health != 5 || state.board.tiles[30][12].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].color != 0x7F) return 1;
	if (strcmp(state.board.info.message, expected_message) != 0
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 20) return 2;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_HEALTH) return 3;
	if (state.sound.current_priority != 4 || state.sound.buffer_len != sizeof(nonlethal_sound)) return 4;
	for (i = 0; i < (int) sizeof(nonlethal_sound); i++) {
		if (state.sound.buffer[i] != nonlethal_sound[i]) return 5 + i;
	}

	reset_movement_state("");
	state.board.stat_count = 0;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = 0x1F;
	state.world.info.health = 5;
	state.game_state = GS_PLAY;
	state.tick_duration = 4;
	state.sound.current_priority = -1;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_board_damage_stat(&state, 0);
	if (state.world.info.health != 0 || state.board.tiles[30][12].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].color != 0x7F) return 20;
	if (strcmp(state.board.info.message, expected_message) != 0
		|| state.board.stat_count != 1 || state.board.stats[1].p2 != 20) return 21;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_HEALTH) return 22;
	if (state.sound.current_priority != 5 || state.sound.buffer_len != sizeof(lethal_sound)) return 23;
	for (i = 0; i < (int) sizeof(lethal_sound); i++) {
		if (state.sound.buffer[i] != lethal_sound[i]) return 24 + i;
	}

	reset_movement_state("");
	state.board.stat_count = 1;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.stats[1].x = 11;
	state.board.stats[1].y = 10;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0x70;
	state.board.tiles[10][10].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[11][10].element = ZOO_E_SLIME;
	state.board.tiles[11][10].color = 0x3A;
	state.tick_duration = 4;
	state.sound.current_priority = -1;
	zoo_element_defs[ZOO_E_SLIME].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.board.stat_count != 0 || state.board.tiles[11][10].element != ZOO_E_BREAKABLE
		|| state.board.tiles[11][10].color != 0x3A) return 60;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(slime_sound)) return 61;
	for (i = 0; i < (int) sizeof(slime_sound); i++) {
		if (state.sound.buffer[i] != slime_sound[i]) return 62 + i;
	}
	if (dx != 2 || dy != -1) return 70;
	return 0;
}

static int check_passage_touch(void) {
	static const uint8_t expected_sound[] = {
		48, 1, 52, 1, 55, 1, 49, 1, 53, 1, 56, 1, 50, 1, 54, 1,
		57, 1, 51, 1, 55, 1, 58, 1, 52, 1, 56, 1, 64, 1
	};
	int16_t dx = -1;
	int16_t dy = 1;
	int16_t i;
	int result;

	reset_movement_state("");
	zoo_board_create(&state);
	state.world.board_count = 1;
	state.world.info.current_board = 0;
	state.board.tiles[11][10].element = ZOO_E_PASSAGE;
	state.board.tiles[11][10].color = 0x2A;
	state.board.stat_count = 1;
	state.board.stats[1] = zoo_stat_template_default;
	state.board.stats[1].x = 11;
	state.board.stats[1].y = 10;
	state.board.stats[1].p3 = 1;
	state.board.stats[1].under.element = ZOO_E_EMPTY;
	state.board.stats[1].under.color = 0;
	result = zoo_board_close(&state);
	if (result != 0) return 1;

	zoo_board_create(&state);
	state.world.info.current_board = 1;
	state.board.tiles[20][15].element = ZOO_E_PASSAGE;
	state.board.tiles[20][15].color = 0x2A;
	state.board.tiles[22][17].element = ZOO_E_PASSAGE;
	state.board.tiles[22][17].color = 0x2A;
	result = zoo_board_close(&state);
	if (result != 0) return 2;
	result = zoo_board_open(&state, 0);
	if (result != 0) return 3;

	state.world.info.board_time_sec = 87;
	state.game_paused = false;
	state.tick_duration = 4;
	state.sound.current_priority = -1;
	state.func_draw_sidebar = test_sidebar_update;
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		state.transition_order[i] = (uint16_t) i;
	}
	zoo_element_defs[ZOO_E_PASSAGE].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.current_board != 1 || state.board.stats[0].x != 22
		|| state.board.stats[0].y != 17) return 4;
	if (state.board.info.start_player_x != 22 || state.board.info.start_player_y != 17
		|| state.world.info.board_time_sec != 0 || !state.game_paused) return 5;
	if (state.board.tiles[30][12].element != ZOO_E_EMPTY || state.board.tiles[30][12].color != 0) return 6;
	if (state.board.tiles[20][15].element != ZOO_E_PASSAGE || state.board.tiles[20][15].color != 0x2A
		|| state.board.tiles[22][17].element != ZOO_E_PASSAGE || state.board.tiles[22][17].color != 0x2A) return 7;
	if (dx != 0 || dy != 0 || state.sound.current_priority != 4
		|| state.sound.buffer_len != sizeof(expected_sound)) return 8;
	for (i = 0; i < (int16_t) sizeof(expected_sound); i++) {
		if (state.sound.buffer[i] != expected_sound[i]) return 9 + i;
	}
	return 0;
}

static int check_gem_touch_negative_health(void) {
	int16_t dx = 4;
	int16_t dy = -3;
	reset_movement_state("");
	state.game_state = GS_PLAY;
	state.board.stats[0].x = 3;
	state.board.stats[0].y = 3;
	state.board.tiles[3][3].element = ZOO_E_PLAYER;
	state.world.info.gems = 41;
	state.world.info.health = -2;
	state.world.info.score = 99;
	state.board.tiles[11][10].element = ZOO_E_GEM;
	state.board.tiles[11][10].color = 0x2F;
	state.msg_flags.gem = false;
	state.func_draw_sidebar = test_sidebar_update;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;

	zoo_element_defs[ZOO_E_GEM].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.gems != 42 || state.world.info.health != 0
		|| state.world.info.score != 109) return 1;
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x2F) return 2;
	if (sidebar_update_count != 1 || sidebar_update_flags !=
		(ZOO_SIDEBAR_UPDATE_GEMS | ZOO_SIDEBAR_UPDATE_HEALTH | ZOO_SIDEBAR_UPDATE_SCORE)) return 3;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != 8
		|| state.sound.buffer[0] != 0x40 || state.sound.buffer[1] != 0x01
		|| state.sound.buffer[6] != 0x30 || state.sound.buffer[7] != 0x01) return 4;
	if (state.msg_flags.gem || dx != 4 || dy != -3) return 5;

	reset_movement_state("");
	state.world.info.health = -2;
	state.board.tiles[11][10].element = ZOO_E_GEM;
	state.msg_flags.gem = false;
	state.func_draw_sidebar = test_sidebar_update;
	zoo_element_defs[ZOO_E_GEM].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.health != -1) return 6;
	return 0;
}

static int check_black_key_gems_alias(void) {
	static const uint8_t duplicate_sound[] = { 0x30, 0x02, 0x20, 0x02 };
	static const uint8_t collect_sound[] = {
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x50, 0x02
	};
	static const uint8_t door_sound[] = {
		0x30, 0x01, 0x37, 0x01, 0x3B, 0x01,
		0x30, 0x01, 0x37, 0x01, 0x3B, 0x01,
		0x40, 0x04
	};
	static const uint8_t locked_door_sound[] = { 0x17, 0x01, 0x10, 0x01 };
	int16_t dx = 2;
	int16_t dy = -1;

	// In Pascal, Keys[1..7] immediately follows the 16-bit Gems field in
	// TWorldInfo. With range checking off, Keys[0] aliases Gems' high byte.
	reset_movement_state("");
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.gems = 0x00FA;
	state.board.tiles[11][10].element = ZOO_E_KEY;
	state.board.tiles[11][10].color = 0x20;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_element_defs[ZOO_E_KEY].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.gems != 0x01FA) return 1;
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY) return 2;
	if (state.board.info.io_message_length != 27
		|| strcmp(state.board.info.message, "You now have the Black key.") != 0) return 3;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != sizeof(collect_sound)
		|| memcmp(state.sound.buffer, collect_sound, sizeof(collect_sound)) != 0) return 4;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_KEYS) return 5;
	for (int i = 0; i < 7; i++) {
		if (state.world.info.keys[i] || state.world.info.io_key_bytes[i] != 0) return 6;
	}

	// A nonzero (including noncanonical) high byte means the out-of-range
	// Boolean already reads true; the key stays and the duplicate sound plays.
	reset_movement_state("");
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.gems = 0x02FA;
	state.board.tiles[11][10].element = ZOO_E_KEY;
	state.board.tiles[11][10].color = 0x20;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_element_defs[ZOO_E_KEY].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.gems != 0x02FA
		|| state.board.tiles[11][10].element != ZOO_E_KEY) return 10;
	if (state.board.info.io_message_length != 29
		|| strcmp(state.board.info.message, "You already have a Black key!") != 0) return 11;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != sizeof(duplicate_sound)
		|| memcmp(state.sound.buffer, duplicate_sound, sizeof(duplicate_sound)) != 0) return 12;
	if (sidebar_update_count != 0) return 13;

	reset_movement_state("");
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.gems = 0x00FA;
	state.board.tiles[11][10].element = ZOO_E_DOOR;
	state.board.tiles[11][10].color = 0x05;
	sidebar_update_count = 0;
	zoo_element_defs[ZOO_E_DOOR].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.gems != 0x00FA
		|| state.board.tiles[11][10].element != ZOO_E_DOOR) return 15;
	if (state.board.info.io_message_length != 25
		|| strcmp(state.board.info.message, "The Black door is locked!") != 0) return 16;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(locked_door_sound)
		|| memcmp(state.sound.buffer, locked_door_sound, sizeof(locked_door_sound)) != 0) return 17;
	if (sidebar_update_count != 0) return 18;

	// The Black door's Keys[0] := false clears the high byte while retaining
	// the low-byte gem count, even when an imported world's high byte is 2.
	reset_movement_state("");
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.gems = 0x02FA;
	state.board.tiles[11][10].element = ZOO_E_DOOR;
	state.board.tiles[11][10].color = 0x05;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_element_defs[ZOO_E_DOOR].touch_func(&state, 11, 10, 0, &dx, &dy);
	if (state.world.info.gems != 0x00FA) return 20;
	if (state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.board.tiles[11][10].color != 0x05) return 21;
	if (state.board.info.io_message_length != 27
		|| strcmp(state.board.info.message, "The Black door is now open.") != 0) return 22;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(door_sound)
		|| memcmp(state.sound.buffer, door_sound, sizeof(door_sound)) != 0) return 23;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_KEYS) return 24;
	if (dx != 2 || dy != -1) return 25;
	return 0;
}

static int check_colored_key_indices(void) {
	static const char *collect_messages[8] = {
		"", "You now have the Blue key.", "You now have the Green key.",
		"You now have the Cyan key.", "You now have the Red key.",
		"You now have the Purple key.", "You now have the Yellow key.",
		"You now have the White key."
	};
	static const char *door_messages[8] = {
		"", "The Blue door is now open.", "The Green door is now open.",
		"The Cyan door is now open.", "The Red door is now open.",
		"The Purple door is now open.", "The Yellow door is now open.",
		"The White door is now open."
	};
	static const uint8_t collect_sound[] = {
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x40, 0x01, 0x44, 0x01, 0x47, 0x01,
		0x50, 0x02
	};
	static const uint8_t door_sound[] = {
		0x30, 0x01, 0x37, 0x01, 0x3B, 0x01,
		0x30, 0x01, 0x37, 0x01, 0x3B, 0x01,
		0x40, 0x04
	};
	int16_t dx = 1;
	int16_t dy = -1;

	for (int key = 1; key <= 7; key++) {
		reset_movement_state("");
		state.func_draw_sidebar = test_sidebar_update;
		state.world.info.gems = 42;
		state.board.tiles[11][10].element = ZOO_E_KEY;
		state.board.tiles[11][10].color = (uint8_t) key;
		sidebar_update_count = 0;
		sidebar_update_flags = 0;
		zoo_element_defs[ZOO_E_KEY].touch_func(&state, 11, 10, 0, &dx, &dy);
		if (state.world.info.gems != 42 || state.board.tiles[11][10].element != ZOO_E_EMPTY) return key;
		if (strcmp(state.board.info.message, collect_messages[key]) != 0) return 10 + key;
		if (state.board.info.io_message_length != strlen(collect_messages[key])) return 20 + key;
		if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_KEYS) return 30 + key;
		if (state.sound.current_priority != 2 || state.sound.buffer_len != sizeof(collect_sound)
			|| memcmp(state.sound.buffer, collect_sound, sizeof(collect_sound)) != 0) return 40 + key;
		for (int i = 0; i < 7; i++) {
			bool expected = i == key - 1;
			if (state.world.info.keys[i] != expected
				|| state.world.info.io_key_bytes[i] != (expected ? 1 : 0)) return 50 + key;
		}

		reset_movement_state("");
		state.func_draw_sidebar = test_sidebar_update;
		state.world.info.gems = 42;
		state.world.info.keys[key - 1] = true;
		state.world.info.io_key_bytes[key - 1] = 1;
		state.board.tiles[11][10].element = ZOO_E_DOOR;
		state.board.tiles[11][10].color = (uint8_t) (key << 4);
		sidebar_update_count = 0;
		sidebar_update_flags = 0;
		zoo_element_defs[ZOO_E_DOOR].touch_func(&state, 11, 10, 0, &dx, &dy);
		if (state.world.info.gems != 42 || state.board.tiles[11][10].element != ZOO_E_EMPTY) return 60 + key;
		if (strcmp(state.board.info.message, door_messages[key]) != 0) return 70 + key;
		if (state.board.info.io_message_length != strlen(door_messages[key])) return 80 + key;
		if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_KEYS) return 90 + key;
		if (state.sound.current_priority != 3 || state.sound.buffer_len != sizeof(door_sound)
			|| memcmp(state.sound.buffer, door_sound, sizeof(door_sound)) != 0) return 100 + key;
		for (int i = 0; i < 7; i++) {
			if (state.world.info.keys[i] || state.world.info.io_key_bytes[i] != 0) return 110 + key;
		}
	}
	if (dx != 1 || dy != -1) return 120;
	return 0;
}

static int check_scroll_touch_default_title(void) {
	static const char scroll_script[] = "First line\rSecond line\r";
	static const char scroll_script_with_title[] = "@Scroll custom title\rFirst line\rSecond line\r";
	int16_t dx = 0;
	int16_t dy = 0;

	reset_movement_state(scroll_script);
	state.board.tiles[10][10].element = ZOO_E_SCROLL;
	zoo_element_defs[ZOO_E_SCROLL].touch_func(&state, 10, 10, 0, &dx, &dy);
	if (!state.object_window_request) return 1;
	if (strcmp(state.object_window.title, "Scroll") != 0) return 2;

	reset_movement_state(scroll_script_with_title);
	state.board.tiles[10][10].element = ZOO_E_SCROLL;
	zoo_element_defs[ZOO_E_SCROLL].touch_func(&state, 10, 10, 0, &dx, &dy);
	if (!state.object_window_request) return 3;
	if (strcmp(state.object_window.title, "Scroll custom title") != 0) return 4;
	return 0;
}

static int check_player_torch_tick(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.health = 100;
	state.world.info.torches = 1;
	state.board.info.is_dark = true;
	state.board.stats[0].x = 30;
	state.board.stats[0].y = 12;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0x00;
	state.board.tiles[30][12].element = ZOO_E_PLAYER;
	state.board.tiles[30][12].color = 0x1F;
	state.player_character = '\x02';
	state.input.actions_down[ZOO_ACTION_TORCH] = true;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	if (state.world.info.torches != 0) return 1;
	if (state.world.info.torch_ticks != ZOO_TORCH_DURATION - 1) return 2;
	if (state.board.stats[0].x != 30 || state.board.stats[0].y != 12) return 3;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_TORCHES) return 4;
	if (state.input.actions_down[ZOO_ACTION_TORCH]) return 5;
	return 0;
}

static void reset_player_shoot_state(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.world.info.health = 100;
	state.world.info.ammo = 2;
	state.board.info.max_shots = 2;
	state.board.stats[0].x = 10;
	state.board.stats[0].y = 10;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[10][10].element = ZOO_E_PLAYER;
	state.board.tiles[10][10].color = 0x1F;
	state.player_character = '\x02';
	state.tick_duration = 4;
	state.sound.current_priority = -1;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
}

static int check_player_shoot_tick(void) {
	reset_player_shoot_state();
	state.input.actions_held[ZOO_ACTION_SHOOT] = true;
	state.input.delta_x = 1;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_shoot_state(0, 11, 10, true);
	if (state.world.info.ammo != 1 || state.input.delta_x != 0 || state.input.delta_y != 0
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 1;
	if (state.board.stat_count != 1 || state.board.tiles[11][10].element != ZOO_E_BULLET
		|| state.board.tiles[11][10].color != 0x0F) return 2;
	if (state.board.stats[1].x != 11 || state.board.stats[1].y != 10
		|| state.board.stats[1].p1 != 0
		|| state.board.stats[1].step_x != 1 || state.board.stats[1].step_y != 0
		|| state.board.stats[1].p2 != 100
		|| state.board.stats[1].under.element != ZOO_E_EMPTY
		|| state.board.stats[1].under.color != 0) return 3;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != 6
		|| state.sound.buffer[0] != 64 || state.sound.buffer[1] != 1
		|| state.sound.buffer[2] != 48 || state.sound.buffer[3] != 1
		|| state.sound.buffer[4] != 32 || state.sound.buffer[5] != 1) return 4;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_AMMO) return 5;

	reset_player_shoot_state();
	state.player_dir_y = 1;
	state.input.shoot_tap = true;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_shoot_state(1, 10, 11, true);
	if (state.world.info.ammo != 1 || state.input.shoot_tap
		|| state.input.delta_x != 0 || state.input.delta_y != 0
		|| state.player_dir_x != 0 || state.player_dir_y != 1) return 6;
	if (state.board.stat_count != 1 || state.board.tiles[10][11].element != ZOO_E_BULLET
		|| state.board.stats[1].step_x != 0 || state.board.stats[1].step_y != 1) return 7;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != 6) return 8;

	reset_player_shoot_state();
	state.input.actions_held[ZOO_ACTION_SHOOT] = true;
	state.input.delta_x = 1;
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	state.board.tiles[11][10].color = 0x1F;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_shoot_state(2, 11, 10, false);
	if (state.world.info.ammo != 2 || state.input.delta_x != 1
		|| state.input.delta_y != 0 || state.player_dir_x != 1 || state.player_dir_y != 0) return 9;
	if (state.board.stat_count != 0 || state.board.tiles[11][10].element != ZOO_E_SOLID
		|| state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 10;

	reset_player_shoot_state();
	state.board.info.max_shots = 1;
	zoo_stat_add(&state, 15, 12, ZOO_E_BULLET,
		zoo_element_defs[ZOO_E_BULLET].color, 1, &zoo_stat_template_default);
	state.board.stats[1].p1 = 0;
	state.input.actions_held[ZOO_ACTION_SHOOT] = true;
	state.input.delta_x = 1;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_shoot_state(3, 11, 10, false);
	if (state.world.info.ammo != 2 || state.input.delta_x != 1
		|| state.board.stat_count != 1 || state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 11;

	reset_player_shoot_state();
	state.input.shoot_tap = true;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_shoot_state(4, 11, 10, false);
	if (state.world.info.ammo != 2 || state.input.shoot_tap
		|| state.board.stat_count != 0 || state.sound.current_priority != -1) return 12;

	return 0;
}

static int check_player_death_tick(void) {
	reset_player_shoot_state();
	state.world.info.health = 0;
	state.input.delta_x = 1;
	state.input.actions_held[ZOO_ACTION_SHOOT] = true;
	state.input.shoot_tap = true;
	state.player_dir_x = 1;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_death_state(0);
	if (state.world.info.health != 0 || state.world.info.ammo != 1
		|| state.input.delta_x != 0 || state.input.delta_y != 0
		|| state.input.actions_held[ZOO_ACTION_SHOOT] || state.input.shoot_tap
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 1;
	if (state.board.stat_count != 2 || state.tick_duration != 0
		|| !state.sound.block_queueing || state.sound.current_priority != -1
		|| state.sound.buffer_len != 0) return 2;
	if (state.board.tiles[0][0].element != ZOO_E_MESSAGE_TIMER
		|| state.board.stats[1].p2 != 0
		|| state.board.tiles[11][10].element != ZOO_E_BULLET
		|| state.board.stats[2].x != 11 || state.board.stats[2].y != 10
		|| state.board.stats[2].p1 != 0 || state.board.stats[2].p2 != 100) return 3;

	reset_player_shoot_state();
	zoo_stat_add(&state, 0, 0, ZOO_E_OBJECT, zoo_element_defs[ZOO_E_OBJECT].color,
		1, &zoo_stat_template_default);
	state.world.info.health = 0;
	state.input.delta_x = 1;
	state.input.actions_held[ZOO_ACTION_SHOOT] = true;
	state.player_dir_x = 0;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_death_state(1);
	if (state.world.info.ammo != 2 || state.input.delta_x != 0 || state.input.delta_y != 0
		|| state.input.actions_held[ZOO_ACTION_SHOOT] || state.player_dir_x != 0) return 4;
	if (state.board.stat_count != 1 || state.tick_duration != 0
		|| !state.sound.block_queueing || state.board.tiles[0][0].element != ZOO_E_OBJECT
		|| state.board.tiles[11][10].element != ZOO_E_EMPTY
		|| state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 5;
	return 0;
}

static int tick_player_movement(void) {
	int16_t x = state.board.stats[0].x + state.input.delta_x;
	int16_t y = state.board.stats[0].y + state.input.delta_y;
	state.player_dir_x = state.input.delta_x;
	state.player_dir_y = state.input.delta_y;
	zoo_element_defs[state.board.tiles[x][y].element].touch_func(&state, x, y, 0,
		&state.input.delta_x, &state.input.delta_y);
	// Enter the production player tick's continuation after the touch callback,
	// matching the second phase of Pascal's synchronous PlayerTick routine.
	state.call_stack.state = 1;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	if (state.call_stack.call != NULL || state.call_stack.curr_call != NULL
		|| state.call_stack.state != 0) return 1;
	if (state.error_value != 0) return 2;
	return 0;
}

static void capture_player_movement_state(int case_index) {
	int16_t *out = &player_movement_outputs[case_index * 17];
	int tile_x;
	*out++ = state.board.stats[0].x;
	*out++ = state.board.stats[0].y;
	*out++ = state.input.delta_x;
	*out++ = state.input.delta_y;
	*out++ = state.player_dir_x;
	*out++ = state.player_dir_y;
	*out++ = state.board.stat_count;
	for (tile_x = 10; tile_x <= 12; tile_x++) {
		*out++ = state.board.tiles[tile_x][10].element;
		*out++ = state.board.tiles[tile_x][10].color;
	}
	*out++ = state.board.stats[0].under.element;
	*out++ = state.board.stats[0].under.color;
	*out++ = state.sound.current_priority;
	*out = state.sound.buffer_len;
}

static void capture_player_shoot_state(int case_index, int target_x, int target_y, bool has_bullet) {
	int16_t *out = &player_shoot_outputs[case_index * 24];
	int field_count = 0;
	const zoo_stat *bullet;
	int i;
	out[field_count++] = state.world.info.ammo;
	out[field_count++] = state.input.delta_x;
	out[field_count++] = state.input.delta_y;
	out[field_count++] = state.player_dir_x;
	out[field_count++] = state.player_dir_y;
	out[field_count++] = state.board.stat_count;
	out[field_count++] = state.board.tiles[target_x][target_y].element;
	out[field_count++] = state.board.tiles[target_x][target_y].color;
	out[field_count++] = state.sound.current_priority;
	out[field_count++] = state.sound.buffer_len;
	if (!has_bullet) return;
	bullet = &state.board.stats[state.board.stat_count];
	out[field_count++] = bullet->x;
	out[field_count++] = bullet->y;
	out[field_count++] = bullet->p1;
	out[field_count++] = bullet->step_x;
	out[field_count++] = bullet->step_y;
	out[field_count++] = bullet->p2;
	out[field_count++] = bullet->under.element;
	out[field_count++] = bullet->under.color;
	for (i = 0; i < state.sound.buffer_len && i < 6; i++) {
		out[field_count++] = state.sound.buffer[i];
	}
}

static void capture_player_key_state(int case_index, bool include_sound_bytes) {
	int16_t *out = &player_key_outputs[case_index * 12];
	int field_count = 0;
	if (include_sound_bytes) {
		out[field_count++] = state.game_paused;
		out[field_count++] = state.world.info.health;
		out[field_count++] = state.world.info.torch_ticks;
		out[field_count++] = state.world.info.energizer_ticks;
		out[field_count++] = state.tick_duration;
		out[field_count++] = state.board.tiles[10][10].color;
		out[field_count++] = (uint8_t) state.player_character;
		out[field_count++] = state.input.key_pressed;
		out[field_count++] = state.sound.current_priority;
		out[field_count++] = state.sound.buffer_len;
		out[field_count++] = state.sound.buffer_len > 0 ? state.sound.buffer[0] : 0;
		out[field_count] = state.sound.buffer_len > 1 ? state.sound.buffer[1] : 0;
	} else {
		out[field_count++] = state.game_paused;
		out[field_count++] = state.world.info.health;
		out[field_count++] = state.sound.enabled;
		out[field_count++] = state.sound.is_playing;
		out[field_count++] = state.sound.buffer_len;
		out[field_count++] = state.sound.current_priority;
		out[field_count] = state.input.key_pressed;
	}
}

static void capture_player_death_state(int case_index) {
	int16_t *out = &player_death_outputs[case_index * 18];
	*out++ = state.board.stats[0].x;
	*out++ = state.board.stats[0].y;
	*out++ = state.world.info.health;
	*out++ = state.world.info.ammo;
	*out++ = state.input.delta_x;
	*out++ = state.input.delta_y;
	*out++ = state.input.actions_held[ZOO_ACTION_SHOOT];
	*out++ = state.player_dir_x;
	*out++ = state.player_dir_y;
	*out++ = state.board.stat_count;
	*out++ = state.tick_duration;
	*out++ = state.sound.block_queueing;
	*out++ = state.sound.current_priority;
	*out++ = state.sound.buffer_len;
	*out++ = state.board.tiles[0][0].element;
	*out++ = state.board.tiles[0][0].color;
	*out++ = state.board.stats[1].p2;
	*out = state.board.tiles[11][10].element;
}

static void reset_player_movement_state(void) {
	reset_player_shoot_state();
	state.board.stats[0].under.color = 0x17;
	state.board.tiles[10][10].color = 0x1F;
	state.player_character = '\x02';
	state.world.info.energizer_ticks = 0;
	state.world.info.torch_ticks = 0;
	state.input.delta_x = 0;
	state.input.delta_y = 0;
	state.input.key_pressed = 0;
	state.sound.enabled = false;
	state.sound.is_playing = false;
	state.sound.current_priority = -1;
	state.sound.buffer_len = 0;
}

static int check_player_movement_tick(void) {
	int result;
	reset_player_movement_state();
	state.board.tiles[11][10].element = ZOO_E_EMPTY;
	state.board.tiles[11][10].color = 0x47;
	state.input.delta_x = 1;
	result = tick_player_movement();
	if (result != 0) return result;
	capture_player_movement_state(0);
	if (state.board.stats[0].x != 11 || state.board.stats[0].y != 10
		|| state.input.delta_x != 1 || state.input.delta_y != 0
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 3;
	if (state.board.stat_count != 0
		|| state.board.tiles[10][10].element != ZOO_E_EMPTY || state.board.tiles[10][10].color != 0x17
		|| state.board.tiles[11][10].element != ZOO_E_PLAYER || state.board.tiles[11][10].color != 0x1F
		|| state.board.tiles[12][10].element != ZOO_E_EMPTY || state.board.tiles[12][10].color != 0
		|| state.board.stats[0].under.element != ZOO_E_EMPTY || state.board.stats[0].under.color != 0x47) return 4;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 5;

	reset_player_movement_state();
	state.board.tiles[11][10].element = ZOO_E_SOLID;
	state.board.tiles[11][10].color = 0x5F;
	state.input.delta_x = 1;
	result = tick_player_movement();
	if (result != 0) return result + 10;
	capture_player_movement_state(1);
	if (state.board.stats[0].x != 10 || state.board.stats[0].y != 10
		|| state.input.delta_x != 1 || state.input.delta_y != 0
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 13;
	if (state.board.tiles[10][10].element != ZOO_E_PLAYER || state.board.tiles[10][10].color != 0x1F
		|| state.board.tiles[11][10].element != ZOO_E_SOLID || state.board.tiles[11][10].color != 0x5F
		|| state.board.tiles[12][10].element != ZOO_E_EMPTY || state.board.tiles[12][10].color != 0
		|| state.board.stats[0].under.element != ZOO_E_EMPTY || state.board.stats[0].under.color != 0x17) return 14;
	if (state.sound.current_priority != -1 || state.sound.buffer_len != 0) return 15;

	reset_player_movement_state();
	state.board.tiles[11][10].element = ZOO_E_BOULDER;
	state.board.tiles[11][10].color = 0x0E;
	state.board.tiles[12][10].element = ZOO_E_EMPTY;
	state.board.tiles[12][10].color = 0x56;
	state.input.delta_x = 1;
	result = tick_player_movement();
	if (result != 0) return result + 20;
	capture_player_movement_state(2);
	if (state.board.stats[0].x != 11 || state.board.stats[0].y != 10
		|| state.input.delta_x != 1 || state.input.delta_y != 0
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 23;
	if (state.board.stat_count != 0
		|| state.board.tiles[10][10].element != ZOO_E_EMPTY || state.board.tiles[10][10].color != 0x17
		|| state.board.tiles[11][10].element != ZOO_E_PLAYER || state.board.tiles[11][10].color != 0x1F
		|| state.board.tiles[12][10].element != ZOO_E_BOULDER || state.board.tiles[12][10].color != 0x0E
		|| state.board.stats[0].under.element != ZOO_E_EMPTY || state.board.stats[0].under.color != 0x0E) return 24;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != 2
		|| state.sound.buffer[0] != 21 || state.sound.buffer[1] != 1) return 25;

	reset_player_movement_state();
	state.board.tiles[11][10].element = ZOO_E_BOULDER;
	state.board.tiles[11][10].color = 0x0E;
	state.board.tiles[12][10].element = ZOO_E_SOLID;
	state.board.tiles[12][10].color = 0x5F;
	state.input.delta_x = 1;
	result = tick_player_movement();
	if (result != 0) return result + 30;
	capture_player_movement_state(3);
	if (state.board.stats[0].x != 10 || state.board.stats[0].y != 10
		|| state.input.delta_x != 1 || state.input.delta_y != 0
		|| state.player_dir_x != 1 || state.player_dir_y != 0) return 33;
	if (state.board.tiles[10][10].element != ZOO_E_PLAYER || state.board.tiles[10][10].color != 0x1F
		|| state.board.tiles[11][10].element != ZOO_E_BOULDER || state.board.tiles[11][10].color != 0x0E
		|| state.board.tiles[12][10].element != ZOO_E_SOLID || state.board.tiles[12][10].color != 0x5F
		|| state.board.stats[0].under.element != ZOO_E_EMPTY || state.board.stats[0].under.color != 0x17) return 34;
	if (state.sound.current_priority != 2 || state.sound.buffer_len != 2
		|| state.sound.buffer[0] != 21 || state.sound.buffer[1] != 1) return 35;
	return 0;
}

static int check_player_pause_key_tick(void) {
	reset_player_shoot_state();
	state.game_state = GS_PLAY;
	state.game_paused = false;
	state.current_tick = 1;
	state.world.info.health = 100;
	state.world.info.torch_ticks = 1;
	state.world.info.energizer_ticks = 1;
	state.input.key_pressed = 'p';
	state.player_character = '\x02';
	state.board.tiles[10][10].color = 0x1F;
	state.sound.enabled = false;
	state.sound.is_playing = false;
	state.sound.current_priority = -1;
	state.sound.block_queueing = false;
	state.sound.buffer_len = 0;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_key_state(0, true);
	if (!state.game_paused || state.world.info.health != 100
		|| state.world.info.torch_ticks != 0 || state.world.info.energizer_ticks != 0
		|| state.tick_duration != 4 || state.board.tiles[10][10].color != 0x1F
		|| state.player_character != '\x01' || state.input.key_pressed != 'p') return 1;
	if (state.sound.current_priority != 3 || state.sound.buffer_len != 6
		|| state.sound.buffer[0] != 48 || state.sound.buffer[1] != 1
		|| state.sound.buffer[2] != 32 || state.sound.buffer[3] != 1
		|| state.sound.buffer[4] != 16 || state.sound.buffer[5] != 1) return 2;
	return 0;
}

static int check_player_sound_toggle_key_tick(void) {
	reset_player_shoot_state();
	state.game_state = GS_PLAY;
	state.game_paused = false;
	state.input.key_pressed = 'b';
	state.sound.enabled = false;
	state.sound.is_playing = true;
	state.sound.current_priority = 2;
	state.sound.buffer_len = 4;
	state.sound.buffer[0] = 1;
	state.sound.buffer[1] = 2;
	state.sound.buffer[2] = 3;
	state.sound.buffer[3] = 4;
	sidebar_update_count = 0;
	sidebar_update_flags = 0;
	zoo_element_defs[ZOO_E_PLAYER].tick_func(&state, 0);
	capture_player_key_state(1, false);
	if (state.game_paused || state.world.info.health != 100
		|| !state.sound.enabled || state.sound.is_playing || state.sound.buffer_len != 0
		|| state.sound.current_priority != 2 || state.input.key_pressed != ' ') return 1;
	if (sidebar_update_count != 1 || sidebar_update_flags != ZOO_SIDEBAR_UPDATE_ALL) return 2;
	return 0;
}

static void reset_paused_player_tick(bool blocked) {
	reset_player_shoot_state();
	state.func_random = test_zero_random;
	state.game_state = GS_PLAY;
	state.game_paused = true;
	state.world.info.health = 0;
	state.world.info.is_save = false;
	state.world.info.io_is_save_byte = 0;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.color = 0x17;
	state.board.tiles[11][10].element = blocked ? ZOO_E_SOLID : ZOO_E_EMPTY;
	state.board.tiles[11][10].color = blocked ? 0x5F : 0x46;
	zoo_input_key_press(&state.input, '6');
	zoo_input_action_down(&state.input, ZOO_ACTION_RIGHT);
	zoo_input_action_up(&state.input, ZOO_ACTION_RIGHT);
	force_elapsed_for_tick_check = true;
}

static int check_paused_player_tick(void) {
	zoo_tick_retval result;
	int16_t *cycle_output;

	reset_paused_player_tick(false);
	result = zoo_tick(&state);
	if (result != RETURN_IMMEDIATE || state.board.stats[0].x != 10
		|| state.board.stats[0].y != 10 || state.input.delta_x != 1
		|| state.input.delta_y != 0 || state.game_tick_state != 1
		|| state.input.actions_down[ZOO_ACTION_RIGHT]) return 1;
	result = zoo_tick(&state);
	if (result != RETURN_NEXT_CYCLE || state.board.stats[0].x != 11
		|| state.board.stats[0].y != 10 || state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x17
		|| state.board.tiles[11][10].element != ZOO_E_PLAYER
		|| state.board.tiles[11][10].color != 0x1F
		|| state.board.stats[0].under.element != ZOO_E_EMPTY
		|| state.board.stats[0].under.color != 0x46 || state.game_paused
		|| !state.world.info.is_save || state.world.info.io_is_save_byte != 1
		|| state.current_stat_tick != state.board.stat_count + 1) return 2;
	cycle_output = player_paused_move_output;
	cycle_output[0] = state.board.stats[0].x;
	cycle_output[1] = state.board.stats[0].y;
	cycle_output[2] = state.board.tiles[10][10].element;
	cycle_output[3] = state.board.tiles[10][10].color;
	cycle_output[4] = state.board.tiles[11][10].element;
	cycle_output[5] = state.board.tiles[11][10].color;
	cycle_output[6] = state.board.stats[0].under.element;
	cycle_output[7] = state.board.stats[0].under.color;
	cycle_output[8] = state.input.delta_x;
	cycle_output[9] = state.input.delta_y;
	cycle_output[10] = state.game_paused;
	cycle_output[11] = state.world.info.is_save;
	cycle_output[12] = state.input.key_pressed;
	cycle_output[13] = state.world.info.health;
	result = zoo_tick(&state);
	if (result != RETURN_NEXT_CYCLE || state.board.stats[0].x != 11
		|| state.board.stats[0].y != 10 || state.input.delta_x != 0
		|| state.input.delta_y != 0 || state.current_stat_tick != 0
		|| state.board.tiles[10][10].element != ZOO_E_EMPTY
		|| state.board.tiles[10][10].color != 0x17
		|| state.board.tiles[11][10].element != ZOO_E_PLAYER
		|| state.board.tiles[11][10].color != 0x1F
		|| state.board.stats[0].under.element != ZOO_E_EMPTY
		|| state.board.stats[0].under.color != 0x46 || state.game_paused
		|| !state.world.info.is_save) return 3;
	cycle_output = player_paused_move_output + 14;
	cycle_output[0] = state.board.stats[0].x;
	cycle_output[1] = state.board.stats[0].y;
	cycle_output[2] = state.board.tiles[10][10].element;
	cycle_output[3] = state.board.tiles[10][10].color;
	cycle_output[4] = state.board.tiles[11][10].element;
	cycle_output[5] = state.board.tiles[11][10].color;
	cycle_output[6] = state.board.stats[0].under.element;
	cycle_output[7] = state.board.stats[0].under.color;
	cycle_output[8] = state.input.delta_x;
	cycle_output[9] = state.input.delta_y;
	cycle_output[10] = state.game_paused;
	cycle_output[11] = state.world.info.is_save;
	cycle_output[12] = state.input.key_pressed;
	cycle_output[13] = state.world.info.health;

	reset_paused_player_tick(true);
	result = zoo_tick(&state);
	if (result != RETURN_IMMEDIATE || state.board.stats[0].x != 10
		|| state.input.delta_x != 1 || state.game_tick_state != 1) return 4;
	result = zoo_tick(&state);
	if (result != RETURN_NEXT_CYCLE || state.board.stats[0].x != 10
		|| !state.game_paused || state.world.info.is_save) return 5;
	cycle_output = player_paused_move_output + 28;
	cycle_output[0] = state.board.stats[0].x;
	cycle_output[1] = state.board.stats[0].y;
	cycle_output[2] = state.board.tiles[10][10].element;
	cycle_output[3] = state.board.tiles[10][10].color;
	cycle_output[4] = state.board.tiles[11][10].element;
	cycle_output[5] = state.board.tiles[11][10].color;
	cycle_output[6] = state.board.stats[0].under.element;
	cycle_output[7] = state.board.stats[0].under.color;
	cycle_output[8] = state.input.delta_x;
	cycle_output[9] = state.input.delta_y;
	cycle_output[10] = state.game_paused;
	cycle_output[11] = state.world.info.is_save;
	cycle_output[12] = state.input.key_pressed;
	cycle_output[13] = state.world.info.health;
	result = zoo_tick(&state);
	if (result != RETURN_NEXT_CYCLE) return 61;
	if (state.board.stats[0].x != 10) return 62;
	if (!state.game_paused) return 63;
	if (state.input.delta_x != 0) return 64;
	if (state.input.delta_y != 0) return 65;
	if (state.world.info.is_save) return 66;

	force_elapsed_for_tick_check = false;
	return 0;
}

static int32_t check_oop_bind_tail_case(uint32_t request_size) {
	static char small_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char large_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r123456789";
	static char target_source[] = "@TARGET\r#CYCLE 9\r#END\r";
	char replacement[70];
	char tail_source[64];
	zoo_stat template_stat;
	uint32_t small_offset, large_offset, target_offset;
	uint32_t tail_size = request_size < 63 ? 63 - request_size : 1;
	int16_t position = 0;
	char *first_data;
	char *second_data;
	uint32_t expected_tail_delta = request_size;
	bool expected_tail_reuse = request_size >= 26 && request_size <= 62;
	bool expected_large_block_reuse = request_size == 27;

	zoo_board_data_heap_reset(&state.board);
	reset_movement_state(small_source);
	state.board.stat_count = 5;
	state.board.stats[1].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(small_source) - 1), &small_offset);
	if (state.board.stats[1].data == NULL) return 1;
	memcpy(state.board.stats[1].data, small_source, sizeof(small_source) - 1);
	state.board.stats[1].data_len = (int16_t) (sizeof(small_source) - 1);
	state.board.stats[1].data_in_heap = true;
	state.board.stats[1].data_heap_offset = small_offset;
	state.board.stats[2].x = 12;
	state.board.stats[2].y = 10;
	state.board.stats[2].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(large_source) - 1), &large_offset);
	if (state.board.stats[2].data == NULL) return 2;
	memcpy(state.board.stats[2].data, large_source, sizeof(large_source) - 1);
	state.board.stats[2].data_len = (int16_t) (sizeof(large_source) - 1);
	state.board.stats[2].data_in_heap = true;
	state.board.stats[2].data_heap_offset = large_offset;
	state.board.stats[3].x = 11;
	state.board.stats[3].y = 10;
	state.board.stats[3].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(target_source) - 1), &target_offset);
	if (state.board.stats[3].data == NULL) return 3;
	memcpy(state.board.stats[3].data, target_source, sizeof(target_source) - 1);
	state.board.stats[3].data_len = (int16_t) (sizeof(target_source) - 1);
	state.board.stats[3].data_in_heap = true;
	state.board.stats[3].data_heap_offset = target_offset;
	state.board.stats[4].x = 13;
	state.board.stats[4].y = 10;
	state.board.stats[4].data = state.board.stats[1].data;
	state.board.stats[4].data_len = state.board.stats[1].data_len;
	state.board.stats[4].data_in_heap = true;
	state.board.stats[4].data_heap_offset = small_offset;
	state.board.stats[5].x = 14;
	state.board.stats[5].y = 10;
	state.board.stats[5].data = state.board.stats[2].data;
	state.board.stats[5].data_len = state.board.stats[2].data_len;
	state.board.stats[5].data_in_heap = true;
	state.board.stats[5].data_heap_offset = large_offset;
	state.board.tiles[11][10].element = ZOO_E_OBJECT;
	state.board.tiles[12][10].element = ZOO_E_OBJECT;
	state.board.tiles[13][10].element = ZOO_E_OBJECT;
	state.board.tiles[14][10].element = ZOO_E_OBJECT;
	zoo_oop_execute(&state, 1, &position, "Bind tail small block");
	position = 0;
	zoo_oop_execute(&state, 2, &position, "Bind tail large block");

	memset(&template_stat, 0, sizeof(template_stat));
	memset(replacement, 'X', request_size);
	template_stat.data = replacement;
	template_stat.data_len = (int16_t) request_size;
	zoo_stat_add(&state, 15, 10, ZOO_E_OBJECT, 0x0F, 4, &template_stat);
	first_data = state.board.stats[6].data;
	if (request_size <= 63) {
		if (first_data != state.board.data_heap + small_offset) return 4;
	} else if (first_data == state.board.data_heap + small_offset
		|| first_data == state.board.data_heap + large_offset) {
		return 5;
	}

	memset(tail_source, 'Y', sizeof(tail_source));
	template_stat.data = tail_source;
	template_stat.data_len = (int16_t) tail_size;
	zoo_stat_add(&state, 16, 10, ZOO_E_OBJECT, 0x0F, 4, &template_stat);
	second_data = state.board.stats[7].data;
	if ((second_data == first_data + expected_tail_delta)
		!= expected_tail_reuse) return 6;
	if ((second_data == state.board.data_heap + large_offset)
		!= expected_large_block_reuse) return 7;
	return 0;
}

int32_t run_oop_bind_reuse_addstat_checks(void) {
	static char bind_source_one[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char bind_source_two[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char target_source[] = "@TARGET\r#CYCLE 9\r#END\r";
	static char replacement_source_one[] = "XXXXXXXXXXXX\r#CYCLE 7\r#END\r";
	static char replacement_source_two[] = "XXXXXXXXXXXX\r#CYCLE 8\r#END\r";
	zoo_stat template_stat;
	int16_t position = 0;
	uint32_t source_one_offset, source_two_offset, target_offset;

	reset_movement_state(bind_source_one);
	state.board.stat_count = 5;
	state.board.stats[1].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(bind_source_one) - 1), &source_one_offset);
	if (state.board.stats[1].data == NULL) return 20;
	memcpy(state.board.stats[1].data, bind_source_one, sizeof(bind_source_one) - 1);
	state.board.stats[1].data_len = (int16_t) (sizeof(bind_source_one) - 1);
	state.board.stats[1].data_in_heap = true;
	state.board.stats[1].data_heap_offset = source_one_offset;
	state.board.stats[2].x = 12;
	state.board.stats[2].y = 10;
	state.board.stats[2].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(bind_source_two) - 1), &source_two_offset);
	if (state.board.stats[2].data == NULL) return 21;
	memcpy(state.board.stats[2].data, bind_source_two, sizeof(bind_source_two) - 1);
	state.board.stats[2].data_len = (int16_t) (sizeof(bind_source_two) - 1);
	state.board.stats[2].data_in_heap = true;
	state.board.stats[2].data_heap_offset = source_two_offset;
	state.board.stats[3].x = 11;
	state.board.stats[3].y = 10;
	state.board.stats[3].data = zoo_board_data_heap_alloc(&state.board,
		(uint32_t) (sizeof(target_source) - 1), &target_offset);
	if (state.board.stats[3].data == NULL) return 22;
	memcpy(state.board.stats[3].data, target_source, sizeof(target_source) - 1);
	state.board.stats[3].data_len = (int16_t) (sizeof(target_source) - 1);
	state.board.stats[3].data_in_heap = true;
	state.board.stats[3].data_heap_offset = target_offset;
	state.board.stats[4].x = 13;
	state.board.stats[4].y = 10;
	state.board.stats[4].data = state.board.stats[1].data;
	state.board.stats[4].data_len = (int16_t) (sizeof(bind_source_one) - 1);
	state.board.stats[4].data_in_heap = true;
	state.board.stats[4].data_heap_offset = source_one_offset;
	state.board.stats[5].x = 14;
	state.board.stats[5].y = 10;
	state.board.stats[5].data = state.board.stats[2].data;
	state.board.stats[5].data_len = (int16_t) (sizeof(bind_source_two) - 1);
	state.board.stats[5].data_in_heap = true;
	state.board.stats[5].data_heap_offset = source_two_offset;
	state.board.tiles[12][10].element = ZOO_E_OBJECT;
	state.board.tiles[11][10].element = ZOO_E_OBJECT;
	state.board.tiles[12][10].element = ZOO_E_OBJECT;
	state.board.tiles[13][10].element = ZOO_E_EMPTY;
	state.board.tiles[14][10].element = ZOO_E_OBJECT;
	state.board.tiles[15][10].element = ZOO_E_EMPTY;
	state.board.tiles[15][10].color = 0x70;
	state.board.tiles[16][10].element = ZOO_E_EMPTY;
	state.board.tiles[16][10].color = 0x70;

	zoo_oop_execute(&state, 1, &position, "Bind first alias reuse");
	if (state.board.stats[1].data != state.board.stats[3].data || state.board.stats[1].cycle != 9) return 1;
	if (!state.board.stats[4].data_reuse_pending
		|| state.board.stats[4].data != state.board.data_heap + source_one_offset) return 2;
	position = 0;
	zoo_oop_execute(&state, 2, &position, "Bind second alias reuse");
	if (state.board.stats[2].data != state.board.stats[3].data || state.board.stats[2].cycle != 9) return 3;
	if (!state.board.stats[5].data_reuse_pending
		|| state.board.stats[5].data != state.board.data_heap + source_two_offset) return 4;

	memset(&template_stat, 0, sizeof(template_stat));
	template_stat.data = replacement_source_one;
	template_stat.data_len = (int16_t) (sizeof(replacement_source_one) - 1);
	zoo_stat_add(&state, 15, 10, ZOO_E_OBJECT, 0x0F, 4, &template_stat);
	if (state.board.stats[6].data != state.board.data_heap + source_one_offset) return 5;
	if (state.board.stats[4].data_reuse_pending || state.board.stats[4].data_reuse_order != 0) return 6;
	if (!state.board.stats[5].data_reuse_pending || state.board.stats[5].data_reuse_order == 0) return 7;
	template_stat.data = replacement_source_two;
	template_stat.data_len = (int16_t) (sizeof(replacement_source_two) - 1);
	zoo_stat_add(&state, 16, 10, ZOO_E_OBJECT, 0x0F, 4, &template_stat);
	if (state.board.stats[7].data != state.board.data_heap + source_two_offset) return 8;
	if (state.board.stats[5].data_reuse_pending || state.board.stats[5].data_reuse_order != 0) return 9;

	position = 13;
	zoo_oop_execute(&state, 4, &position, "Bind first alias follower after reuse");
	if (state.board.stats[4].cycle != 7) return 10;
	position = 13;
	zoo_oop_execute(&state, 5, &position, "Bind second alias follower after reuse");
	if (state.board.stats[5].cycle != 8) return 11;
	if (state.board.stats[4].data != state.board.stats[6].data
		|| state.board.stats[6].data != state.board.data_heap + source_one_offset) return 12;
	if (state.board.stats[5].data != state.board.stats[7].data
		|| state.board.stats[7].data != state.board.data_heap + source_two_offset) return 13;

	static char small_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char large_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r123456789";
	static char coalesce_target[] = "@TARGET\r#CYCLE 9\r#END\r";
	static const char replacement_small[] = "XCYCLE 7\r#END\r";
	static const char replacement_medium[] = "XXXXXXXXXXXX\r#CYCLE 7\r#END\r";
	static const char replacement_large[] = "XXXXXXXXXXXXXXXXXXXXX\r#CYCLE 7\r#END\r";
	static const uint32_t mixed_sizes[] = {14, 27, 36, 41, 45, 50, 55, 60, 63, 64, 70};
	char replacement_extra[70];
	uint32_t mixed_index;
	for (mixed_index = 0; mixed_index < sizeof(mixed_sizes) / sizeof(mixed_sizes[0]); mixed_index++) {
		uint32_t request_size = mixed_sizes[mixed_index];
		uint32_t small_offset, large_offset, coalesce_target_offset;
		uint32_t replacement_len;
		const char *replacement;
		uint8_t expected_small_byte, expected_large_byte;
		int should_reuse = request_size <= 63;

		zoo_board_data_heap_reset(&state.board);
		reset_movement_state(small_source);
		state.board.stat_count = 5;
		state.board.stats[1].data = zoo_board_data_heap_alloc(&state.board,
			(uint32_t) (sizeof(small_source) - 1), &small_offset);
		if (state.board.stats[1].data == NULL) return 23;
		memcpy(state.board.stats[1].data, small_source, sizeof(small_source) - 1);
		state.board.stats[1].data_len = (int16_t) (sizeof(small_source) - 1);
		state.board.stats[1].data_in_heap = true;
		state.board.stats[1].data_heap_offset = small_offset;
		state.board.stats[2].x = 12;
		state.board.stats[2].y = 10;
		state.board.stats[2].data = zoo_board_data_heap_alloc(&state.board,
			(uint32_t) (sizeof(large_source) - 1), &large_offset);
		if (state.board.stats[2].data == NULL) return 24;
		memcpy(state.board.stats[2].data, large_source, sizeof(large_source) - 1);
		state.board.stats[2].data_len = (int16_t) (sizeof(large_source) - 1);
		state.board.stats[2].data_in_heap = true;
		state.board.stats[2].data_heap_offset = large_offset;
		state.board.stats[3].x = 11;
		state.board.stats[3].y = 10;
		state.board.stats[3].data = zoo_board_data_heap_alloc(&state.board,
			(uint32_t) (sizeof(coalesce_target) - 1), &coalesce_target_offset);
		if (state.board.stats[3].data == NULL) return 25;
		memcpy(state.board.stats[3].data, coalesce_target, sizeof(coalesce_target) - 1);
		state.board.stats[3].data_len = (int16_t) (sizeof(coalesce_target) - 1);
		state.board.stats[3].data_in_heap = true;
		state.board.stats[3].data_heap_offset = coalesce_target_offset;
		state.board.stats[4].data = state.board.stats[1].data;
		state.board.stats[4].data_len = state.board.stats[1].data_len;
		state.board.stats[4].data_in_heap = true;
		state.board.stats[4].data_heap_offset = small_offset;
		state.board.stats[5].data = state.board.stats[2].data;
		state.board.stats[5].data_len = state.board.stats[2].data_len;
		state.board.stats[5].data_in_heap = true;
		state.board.stats[5].data_heap_offset = large_offset;
		state.board.stats[2].x = 12;
		state.board.stats[4].x = 13;
		state.board.stats[5].x = 14;
		state.board.tiles[10][10].element = ZOO_E_OBJECT;
		state.board.tiles[11][10].element = ZOO_E_OBJECT;
		state.board.tiles[12][10].element = ZOO_E_OBJECT;
		state.board.tiles[13][10].element = ZOO_E_OBJECT;
		state.board.tiles[14][10].element = ZOO_E_OBJECT;
		state.board.tiles[15][10].element = ZOO_E_EMPTY;
		state.board.tiles[16][10].element = ZOO_E_EMPTY;
		position = 0;
		zoo_oop_execute(&state, 1, &position, "Bind adjacent small block");
		position = 0;
		zoo_oop_execute(&state, 2, &position, "Bind adjacent large block");

		if (request_size == sizeof(replacement_small) - 1) {
			replacement = replacement_small;
			replacement_len = sizeof(replacement_small) - 1;
		} else if (request_size == sizeof(replacement_medium) - 1) {
			replacement = replacement_medium;
			replacement_len = sizeof(replacement_medium) - 1;
		} else if (request_size == sizeof(replacement_large) - 1) {
			replacement = replacement_large;
			replacement_len = sizeof(replacement_large) - 1;
		} else {
			memset(replacement_extra, 'X', request_size);
			replacement = replacement_extra;
			replacement_len = request_size;
		}
		if (replacement_len != request_size) return 28;
		memset(&template_stat, 0, sizeof(template_stat));
		template_stat.data = (char *) replacement;
		template_stat.data_len = (int16_t) request_size;
		zoo_stat_add(&state, 15, 10, ZOO_E_OBJECT, 0x0F, 4, &template_stat);
		if (state.board.stats[6].data_len != request_size
			|| memcmp(state.board.stats[6].data, replacement, request_size) != 0) return 29;
		if (should_reuse) {
			if (state.board.stats[6].data != state.board.data_heap + small_offset) return 30;
		} else if (state.board.stats[6].data == state.board.data_heap + small_offset
			|| state.board.stats[6].data == state.board.data_heap + large_offset) {
			return 31;
		}
		expected_small_byte = should_reuse ? (uint8_t) replacement[0] : (uint8_t) '#';
		expected_large_byte = (should_reuse && request_size > large_offset - small_offset)
			? (uint8_t) replacement[large_offset - small_offset] : (uint8_t) '#';
		if ((uint8_t) state.board.stats[4].data[0] != expected_small_byte) return 32;
		if ((uint8_t) state.board.stats[5].data[0] != expected_large_byte) return 33;
	}
	for (uint32_t request_size = 26; request_size <= 64; request_size++) {
		int32_t result = check_oop_bind_tail_case(request_size);
		if (result != 0) return 1000 + (int32_t) request_size * 10 + result;
	}
	return 0;
}

static int check_oop_bind_text_window_case(uint32_t source_len, int expected_reuse) {
	static const char bind_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static const char target_source[] = "@TARGET\r#CYCLE 9\r#END\r";
	static const char window_text[] = "01234567890123456789012345678901234567890123456789";
	uint32_t source_offset, target_offset;
	char *source_data, *target_data, *alias_data;
	int16_t position = 0;

	if (state.object_window.line_count > 0) zoo_window_close(&state.object_window);
	zoo_board_data_heap_reset(&state.board);
	reset_movement_state(bind_source);
	state.board.stat_count = 3;
	source_data = zoo_board_data_heap_alloc(&state.board, source_len, &source_offset);
	if (source_data == NULL || source_len < sizeof(bind_source) - 1) return 1;
	memcpy(source_data, bind_source, sizeof(bind_source) - 1);
	memset(source_data + sizeof(bind_source) - 1, 'X', source_len - (sizeof(bind_source) - 1));
	state.board.stats[1].data = source_data;
	state.board.stats[1].data_len = (int16_t) source_len;
	state.board.stats[1].data_in_heap = true;
	state.board.stats[1].data_heap_offset = source_offset;
	alias_data = source_data;

	target_data = zoo_board_data_heap_alloc(&state.board, sizeof(target_source) - 1, &target_offset);
	if (target_data == NULL) return 2;
	memcpy(target_data, target_source, sizeof(target_source) - 1);
	state.board.stats[2].x = 11;
	state.board.stats[2].y = 10;
	state.board.stats[2].data = target_data;
	state.board.stats[2].data_len = (int16_t) (sizeof(target_source) - 1);
	state.board.stats[2].data_in_heap = true;
	state.board.stats[2].data_heap_offset = target_offset;
	state.board.stats[3].x = 12;
	state.board.stats[3].y = 10;
	state.board.stats[3].data = alias_data;
	state.board.stats[3].data_len = (int16_t) source_len;
	state.board.stats[3].data_in_heap = true;
	state.board.stats[3].data_heap_offset = source_offset;
	state.board.tiles[10][10].element = ZOO_E_OBJECT;
	state.board.tiles[11][10].element = ZOO_E_OBJECT;
	state.board.tiles[12][10].element = ZOO_E_OBJECT;

	zoo_oop_execute(&state, 1, &position, "Bind text-window allocation");
	if (state.board.stats[1].cycle != 9 || state.board.stats[1].data != target_data) return 3;
	memset(&state.object_window, 0, sizeof(state.object_window));
	zoo_window_append_pascal(&state, &state.object_window, window_text);
	if (((uint8_t) alias_data[0] == 50) != expected_reuse) return 4;
	if (expected_reuse) {
		if (alias_data[1] != '0' || alias_data[13] != '2') return 5;
		if (state.object_window.line_count != 1
			|| state.object_window.pascal_heap_line_records == NULL
			|| state.object_window.pascal_heap_line_records[0] != alias_data
			|| strcmp(state.object_window.lines[0], window_text) != 0) return 6;
	} else if (alias_data[0] != '#') {
		return 7;
	}
	zoo_window_close(&state.object_window);
	return 0;
}

int32_t run_oop_bind_text_window_reuse_checks(void) {
	int result = check_oop_bind_text_window_case(50, 0);
	if (result != 0) return result;
	result = check_oop_bind_text_window_case(51, 1);
	if (result != 0) return result + 10;
	zoo_board_data_heap_reset(&state.board);
	return 0;
}

int32_t run_player_tick_capture_checks(void) {
	int result;
	memset(player_movement_outputs, 0, sizeof(player_movement_outputs));
	memset(player_shoot_outputs, 0, sizeof(player_shoot_outputs));
	memset(player_key_outputs, 0, sizeof(player_key_outputs));
	memset(player_death_outputs, 0, sizeof(player_death_outputs));
	memset(player_paused_move_output, 0, sizeof(player_paused_move_output));
	result = check_player_movement_tick();
	if (result != 0) return result + 100;
	result = check_paused_player_tick();
	if (result != 0) return result + 150;
	result = check_player_shoot_tick();
	if (result != 0) return result + 200;
	result = check_player_pause_key_tick();
	if (result != 0) return result + 300;
	result = check_player_sound_toggle_key_tick();
	if (result != 0) return result + 400;
	result = check_player_death_tick();
	if (result != 0) return result + 500;
	return 0;
}

uint32_t player_movement_output_ptr(void) {
	return (uint32_t) (uintptr_t) player_movement_outputs;
}

uint32_t player_shoot_output_ptr(void) {
	return (uint32_t) (uintptr_t) player_shoot_outputs;
}

uint32_t player_key_output_ptr(void) {
	return (uint32_t) (uintptr_t) player_key_outputs;
}

uint32_t player_death_output_ptr(void) {
	return (uint32_t) (uintptr_t) player_death_outputs;
}

uint32_t player_paused_move_output_ptr(void) {
	return (uint32_t) (uintptr_t) player_paused_move_output;
}

int32_t run_oop_production_movement_checks(void) {
	static const char try_script[] = "#TRY EAST\r#CYCLE 5\r";
	static const char go_script[] = "#GO EAST\r#CYCLE 5\r";
	static const char try_push_script[] = "#TRY EAST\r#CYCLE 5\r";
	static const char go_push_script[] = "#GO EAST\r#CYCLE 5\r";
	static const char try_chain_script[] = "#TRY EAST\r#CYCLE 5\r";
	static const char go_chain_script[] = "#GO EAST\r#CYCLE 5\r";
	static const char try_blocked_script[] = "#TRY EAST #CYCLE 5\r";
	static const char go_blocked_script[] = "#GO EAST #CYCLE 5\r";
	static const char try_east_script[] = "#TRY EAST #CYCLE 5\r";
	static const char try_south_script[] = "#TRY SOUTH #CYCLE 5\r";
	int result;
	result = check_player_movement_tick();
	if (result != 0) return result + 580;
	result = check_paused_player_tick();
	if (result != 0) return result + 640;
	result = check_player_pause_key_tick();
	if (result != 0) return result + 650;
	result = check_player_sound_toggle_key_tick();
	if (result != 0) return result + 660;
	result = check_player_death_tick();
	if (result != 0) return result + 620;
	result = check_move(try_script);
	if (result != 0) return result;
	result = check_move(go_script);
	if (result != 0) return result + 10;
	result = check_push_move(try_push_script);
	if (result != 0) return result + 20;
	result = check_push_move(go_push_script);
	if (result != 0) return result + 30;
	result = check_boulder_chain(try_chain_script);
	if (result != 0) return result + 40;
	result = check_boulder_chain(go_chain_script);
	if (result != 0) return result + 50;
	result = check_blocked_boulder_chain(try_blocked_script, true);
	if (result != 0) return result + 60;
	result = check_blocked_boulder_chain(go_blocked_script, false);
	if (result != 0) return result + 70;
	result = check_slider_move(try_east_script, ZOO_E_SLIDER_NS, 1, 0, false);
	if (result != 0) return result + 80;
	result = check_slider_move(try_east_script, ZOO_E_SLIDER_EW, 1, 0, true);
	if (result != 0) return result + 90;
	result = check_slider_move(try_south_script, ZOO_E_SLIDER_NS, 0, 1, true);
	if (result != 0) return result + 100;
	result = check_slider_move(try_south_script, ZOO_E_SLIDER_EW, 0, 1, false);
	if (result != 0) return result + 110;
	result = check_duplicator_stat_limit();
	if (result != 0) return result + 120;
	result = check_pusher_chain();
	if (result != 0) return result + 130;
	result = check_bullet_tick();
	if (result != 0) return result + 150;
	result = check_conveyor_rotation();
	if (result != 0) return result + 180;
	result = check_bomb_countdown();
	if (result != 0) return result + 200;
	result = check_bomb_touch();
	if (result != 0) return result + 210;
	result = check_slime_reproduction_interval();
	if (result != 0) return result + 220;
	result = check_centipede_chain_and_reversal();
	if (result != 0) return result + 240;
	result = check_bear_chase();
	if (result != 0) return result + 260;
	result = check_shark_water_only_movement();
	if (result != 0) return result + 280;
	result = check_lion_direction_branches();
	if (result != 0) return result + 300;
	result = check_ruffian_wake_and_stop();
	if (result != 0) return result + 320;
	result = check_tiger_shot_priority();
	if (result != 0) return result + 340;
	result = check_star_tick_period_and_water();
	if (result != 0) return result + 360;
	result = check_energizer_touch();
	if (result != 0) return result + 380;
	result = check_torch_touch();
	if (result != 0) return result + 400;
	result = check_terrain_touches();
	if (result != 0) return result + 410;
	result = check_shared_damage_and_slime_touch();
	if (result != 0) return result + 430;
	result = check_passage_touch();
	if (result != 0) return result + 450;
	result = check_gem_touch_negative_health();
	if (result != 0) return result + 420;
	result = check_black_key_gems_alias();
	if (result != 0) return result + 440;
	result = check_colored_key_indices();
	if (result != 0) return result + 480;
	result = check_scroll_touch_default_title();
	if (result != 0) return result + 520;
	result = check_player_torch_tick();
	if (result != 0) return result + 540;
	result = check_player_shoot_tick();
	if (result != 0) return result + 560;
	return 0;
}

static int capture_integrated_gameplay_snapshot(int checkpoint_index, int16_t checkpoint_kind, int16_t checkpoint_id) {
	int32_t *out;
	int16_t stat_id, alias_id, ix, iy;
	int16_t world_name_length, board_name_length;
	int32_t stat_slots;
	int32_t script_bytes = 0;
	int32_t sound_fields;
	int script_index;
	if (checkpoint_index < 0 || checkpoint_index >= INTEGRATED_GAMEPLAY_CHECKPOINT_CAPACITY
			|| checkpoint_index != integrated_gameplay_snapshot_count_value) return 0;
	out = integrated_gameplay_snapshots[checkpoint_index];
	if (state.board.stat_count < 0 || state.board.stat_count > ZOO_MAX_STAT) return 0;
	stat_slots = (int32_t) state.board.stat_count + 1;
	for (stat_id = 0; stat_id <= state.board.stat_count; stat_id++) {
		zoo_stat *stat = &state.board.stats[stat_id];
		bool earlier_owner = false;
		if (stat->data != NULL && stat->data_len > 0) {
			for (alias_id = 0; alias_id < stat_id; alias_id++) {
				if (state.board.stats[alias_id].data == stat->data
						&& state.board.stats[alias_id].data_len > 0) earlier_owner = true;
			}
			if (!earlier_owner) script_bytes += stat->data_len;
		}
	}
	if (state.sound.buffer_len < 0 || state.sound.buffer_len > 255) return 0;
	world_name_length = state.world.info.io_name_valid
		? state.world.info.io_name_length : 0;
	if (!state.world.info.io_name_valid) {
		while (world_name_length < 20 && state.world.info.name[world_name_length] != '\0')
			world_name_length++;
	}
	board_name_length = state.board.io_name_valid ? state.board.io_name_length : 0;
	if (!state.board.io_name_valid) {
		while (board_name_length < 50 && state.board.name[board_name_length] != '\0')
			board_name_length++;
	}
	if (world_name_length < 0 || world_name_length > 20
			|| board_name_length < 0 || board_name_length > 50) return 0;
	sound_fields = 8 + state.sound.buffer_len;
	if (13 + (14 * stat_slots) + 11 + (62 * 27 * 2) + 2
			+ (stat_slots * stat_slots) + script_bytes + sound_fields + 2 + 3
			+ world_name_length + board_name_length > INTEGRATED_GAMEPLAY_MAX_FIELDS) {
		return 0;
	}

	integrated_gameplay_checkpoint_headers[checkpoint_index * 2] = checkpoint_kind;
	integrated_gameplay_checkpoint_headers[checkpoint_index * 2 + 1] = checkpoint_id;
	*out++ = state.world.info.ammo;
	*out++ = state.world.info.gems;
	*out++ = state.world.info.health;
	*out++ = state.world.info.current_board;
	*out++ = state.world.info.torches;
	*out++ = state.world.info.torch_ticks;
	*out++ = state.world.info.energizer_ticks;
	*out++ = state.world.info.score;
	*out++ = state.world.info.board_time_sec;
	*out++ = state.world.info.board_time_hsec;
	*out++ = state.world.info.is_save ? 1 : 0;
	*out++ = state.board.stat_count;
	*out++ = state.game_state == GS_PLAY ? 1 : (state.game_state == GS_TITLE ? 2 : 0);
	for (stat_id = 0; stat_id <= state.board.stat_count; stat_id++) {
		zoo_stat *stat = &state.board.stats[stat_id];
		*out++ = stat->x;
		*out++ = stat->y;
		*out++ = stat->step_x;
		*out++ = stat->step_y;
		*out++ = stat->cycle;
		*out++ = stat->p1;
		*out++ = stat->p2;
		*out++ = stat->p3;
		*out++ = stat->follower;
		*out++ = stat->leader;
		*out++ = stat->under.element;
		*out++ = stat->under.color;
		*out++ = stat->data_pos;
		*out++ = stat->data_len;
	}
	*out++ = state.current_tick;
	*out++ = state.current_stat_tick;
	*out++ = state.tick_speed;
	*out++ = state.tick_duration;
	*out++ = state.tick_counter;
	*out++ = state.game_paused ? 1 : 0;
	*out++ = state.game_play_exit_requested ? 1 : 0;
	*out++ = state.input.delta_x;
	*out++ = state.input.delta_y;
	*out++ = state.input.key_pressed;
	*out++ = state.input.key_buffer_count;
	for (ix = 0; ix <= ZOO_BOARD_WIDTH + 1; ix++) {
		for (iy = 0; iy <= ZOO_BOARD_HEIGHT + 1; iy++) {
			*out++ = state.board.tiles[ix][iy].element;
			*out++ = state.board.tiles[ix][iy].color;
		}
	}
	*out++ = state.random_seed & 0xFFFFu;
	*out++ = state.random_seed >> 16;
	for (stat_id = 0; stat_id <= state.board.stat_count; stat_id++) {
		for (alias_id = 0; alias_id <= state.board.stat_count; alias_id++) {
			*out++ = state.board.stats[stat_id].data == state.board.stats[alias_id].data ? 1 : 0;
		}
	}
	for (stat_id = 0; stat_id <= state.board.stat_count; stat_id++) {
		zoo_stat *stat = &state.board.stats[stat_id];
		bool earlier_owner = false;
		if (stat->data != NULL && stat->data_len > 0) {
			for (alias_id = 0; alias_id < stat_id; alias_id++) {
				if (state.board.stats[alias_id].data == stat->data
						&& state.board.stats[alias_id].data_len > 0) earlier_owner = true;
			}
			if (!earlier_owner) {
				for (script_index = 0; script_index < stat->data_len; script_index++) {
					*out++ = (uint8_t) stat->data[script_index];
				}
			}
		}
	}
	*out++ = state.sound.current_priority;
	*out++ = state.sound.buffer_pos + 1;
	*out++ = state.sound.buffer_len;
	*out++ = state.sound.is_playing ? 1 : 0;
	*out++ = state.sound.enabled ? 1 : 0;
	*out++ = state.sound.block_queueing ? 1 : 0;
	*out++ = state.sound.duration_counter;
	*out++ = state.sound.duration_multiplier;
	for (script_index = 0; script_index < state.sound.buffer_len; script_index++) {
		*out++ = state.sound.buffer[script_index];
	}
	*out++ = integrated_gameplay_cell_event_count_value;
	*out++ = integrated_gameplay_cell_event_overflow_value;
	*out++ = integrated_gameplay_sound_event_count_value;
	*out++ = world_name_length;
	for (script_index = 0; script_index < world_name_length; script_index++) {
		*out++ = state.world.info.io_name_valid
			? state.world.info.io_name_data[script_index]
			: (uint8_t) state.world.info.name[script_index];
	}
	*out++ = board_name_length;
	for (script_index = 0; script_index < board_name_length; script_index++) {
		*out++ = state.board.io_name_valid
			? state.board.io_name_data[script_index]
			: (uint8_t) state.board.name[script_index];
	}
	integrated_gameplay_snapshot_lengths[checkpoint_index] = (int32_t) (out - integrated_gameplay_snapshots[checkpoint_index]);
	if (integrated_gameplay_snapshot_lengths[checkpoint_index] > INTEGRATED_GAMEPLAY_MAX_FIELDS) return 0;
	integrated_gameplay_snapshot_count_value++;
	return 1;
}

static void integrated_gameplay_capture_reset(void) {
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	memset(integrated_gameplay_snapshots, 0, sizeof(integrated_gameplay_snapshots));
	memset(integrated_gameplay_snapshot_lengths, 0, sizeof(integrated_gameplay_snapshot_lengths));
	memset(integrated_gameplay_checkpoint_headers, 0, sizeof(integrated_gameplay_checkpoint_headers));
	integrated_gameplay_snapshot_count_value = 0;
	memset(integrated_gameplay_sound_events, 0, sizeof(integrated_gameplay_sound_events));
	integrated_gameplay_sound_event_count_value = 0;
	integrated_gameplay_sound_event_overflow_value = 0;
	integrated_gameplay_sound_capture_enabled = false;
	memset(integrated_gameplay_cell_events, 0, sizeof(integrated_gameplay_cell_events));
	memset(integrated_gameplay_cell_context_events, 0, sizeof(integrated_gameplay_cell_context_events));
	integrated_gameplay_cell_event_count_value = 0;
	integrated_gameplay_cell_event_overflow_value = 0;
	integrated_gameplay_cell_capture_enabled = false;
	integrated_gameplay_cell_coalesce_idempotent_writes = false;
	integrated_gameplay_cell_idempotent_write_count_value = 0;
	memset(integrated_gameplay_cell_screen_valid, 0, sizeof(integrated_gameplay_cell_screen_valid));
	memset(integrated_gameplay_cell_screen_color, 0, sizeof(integrated_gameplay_cell_screen_color));
	memset(integrated_gameplay_cell_screen_character, 0, sizeof(integrated_gameplay_cell_screen_character));
	memset(integrated_gameplay_save_bytes, 0, sizeof(integrated_gameplay_save_bytes));
	integrated_gameplay_save_length_value = 0;
	integrated_gameplay_save_result_value = -1;
	integrated_gameplay_save_overflow_value = 0;
	memset(integrated_gameplay_reload_input_bytes, 0, sizeof(integrated_gameplay_reload_input_bytes));
	integrated_gameplay_reload_result_value = -1;
	memset(integrated_gameplay_roundtrip_save_bytes, 0, sizeof(integrated_gameplay_roundtrip_save_bytes));
	integrated_gameplay_roundtrip_save_length_value = 0;
	integrated_gameplay_roundtrip_save_result_value = -1;
	integrated_gameplay_roundtrip_save_overflow_value = 0;
	memset(integrated_gameplay_trace, 0, sizeof(integrated_gameplay_trace));
	force_elapsed_for_tick_check = false;
	integrated_fixed_clock_enabled = false;
	integrated_fixed_clock_hsecs = 0;
	integrated_schedule_enabled = true;
	integrated_elapsed_nanos = 0;
	integrated_configured_tick_speed = 0;
	video.func_write = test_video_write;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	integrated_gameplay_initial_paused = true;
	integrated_gameplay_post_loop_key_pressed = 0;
	/* Match the original Pascal player's transient glyph override. */
	state.player_character = 2;
	zoo_sound_state_init(&state.sound);
	state.sound.enabled = false;
	state.sound.current_priority = -1;
	state.sound.duration_counter = 1;
}

static void integrated_gameplay_set_normalized_input(char command) {
	zoo_input_action_set(&state.input, ZOO_ACTION_UP, false);
	zoo_input_action_set(&state.input, ZOO_ACTION_DOWN, false);
	zoo_input_action_set(&state.input, ZOO_ACTION_LEFT, false);
	zoo_input_action_set(&state.input, ZOO_ACTION_RIGHT, false);
	zoo_input_action_set(&state.input, ZOO_ACTION_SHOOT, false);
	zoo_input_action_set(&state.input, ZOO_ACTION_UP, command == 'U');
	zoo_input_action_set(&state.input, ZOO_ACTION_DOWN, command == 'D');
	zoo_input_action_set(&state.input, ZOO_ACTION_LEFT, command == 'L');
	zoo_input_action_set(&state.input, ZOO_ACTION_RIGHT, command == 'R' || command == 'F');
	zoo_input_action_set(&state.input, ZOO_ACTION_SHOOT, command == 'S' || command == 'F');
}

static int integrated_gameplay_run_ticks(const char *input_tape, int16_t input_length,
	const char *normalized_tape, int16_t normalized_length,
	int16_t cycle_limit, bool normalized_input_at_update, bool use_fixed_clock,
	bool pause_transition_input_guard, bool cycle_clock) {
	int16_t cycles = 0;
	int32_t calls = 0;
	int16_t previous_stat;
	int16_t previous_tick;
	int16_t tick_result;
	int16_t last_board;
	int16_t last_game_state;
	bool last_paused;
	int16_t transition_count = 0;
	zoo_io_handle save_handle;

	integrated_normalized_input_tape = normalized_tape;
	integrated_normalized_input_tape_length = normalized_length;
	integrated_normalized_input_tape_position = 0;
	integrated_normalized_input_at_update = normalized_input_at_update;
	integrated_torch_action_pending = false;
	integrated_cycle_clock_enabled = cycle_clock;
	integrated_fixed_clock_enabled = use_fixed_clock || cycle_clock;
	integrated_fixed_clock_hsecs = cycle_clock ? 0 : 30;
	integrated_capture_first_board_change_checkpoint = normalized_input_at_update
		&& use_fixed_clock && pause_transition_input_guard;
	integrated_board_change_checkpoint_captured = false;
	integrated_board_change_checkpoint_failed = false;
	integrated_pause_transition_checkpoint_pending = pause_transition_input_guard;
	state.tick_speed = integrated_configured_tick_speed;
	state.game_paused = integrated_gameplay_initial_paused;
	if (!capture_integrated_gameplay_snapshot(0, 'L', 0)) return 4;
	force_elapsed_for_tick_check = true;
	zoo_game_start(&state, GS_PLAY);
	/* GamePlayLoop can enter this path unpaused; zoo_game_start defaults GS_PLAY to paused. */
	state.game_paused = integrated_gameplay_initial_paused;
	while (input_length-- > 0) zoo_input_key_press(&state.input, (uint8_t) *input_tape++);
	if (!capture_integrated_gameplay_snapshot(1, 'S', 0)) return 4;
	last_board = state.world.info.current_board;
	last_game_state = state.game_state;
	last_paused = state.game_paused;
	integrated_gameplay_sound_capture_enabled = true;
	integrated_gameplay_cell_capture_enabled = true;

	while (cycles < cycle_limit && calls < 200000) {
		if (!integrated_normalized_input_at_update
				&& integrated_normalized_input_tape != NULL
				&& integrated_normalized_input_tape_position < integrated_normalized_input_tape_length
				&& state.game_tick_state == 0
				&& (state.game_paused || state.current_stat_tick >= state.board.stat_count)) {
			integrated_gameplay_set_normalized_input(
				integrated_normalized_input_tape[integrated_normalized_input_tape_position++]);
		}
		previous_stat = state.current_stat_tick;
		previous_tick = state.current_tick;
		tick_result = (int16_t) zoo_tick(&state);
		/* Pascal's InputUpdate leaves T available until the next PlayerTick.
		   LibZoo represents that key mapping as a one-shot action. Set it after
		   the end-of-cycle zoo_input_clear so the real player callback consumes
		   it on the following cycle, matching the staged Pascal hook boundary. */
		if (integrated_torch_action_pending) {
			zoo_input_action_set(&state.input, ZOO_ACTION_TORCH, true);
			integrated_torch_action_pending = false;
		}
		integrated_gameplay_trace[(calls % 24) * 15 + 0] = previous_stat;
		integrated_gameplay_trace[(calls % 24) * 15 + 1] = state.current_stat_tick;
		integrated_gameplay_trace[(calls % 24) * 15 + 2] = state.current_tick;
		integrated_gameplay_trace[(calls % 24) * 15 + 3] = state.board.stat_count;
		integrated_gameplay_trace[(calls % 24) * 15 + 4] = state.game_tick_state;
		integrated_gameplay_trace[(calls % 24) * 15 + 5] = state.call_stack.call != NULL ? 1 : 0;
		integrated_gameplay_trace[(calls % 24) * 15 + 6] = state.game_paused ? 1 : 0;
		integrated_gameplay_trace[(calls % 24) * 15 + 7] = tick_result;
		integrated_gameplay_trace[(calls % 24) * 15 + 8] = state.board.stats[1].data_pos;
		integrated_gameplay_trace[(calls % 24) * 15 + 9] = integrated_normalized_input_tape_position;
		integrated_gameplay_trace[(calls % 24) * 15 + 10] = state.input.delta_x;
		integrated_gameplay_trace[(calls % 24) * 15 + 11] = state.input.delta_y;
		integrated_gameplay_trace[(calls % 24) * 15 + 12] = state.input.actions_down[ZOO_ACTION_RIGHT] ? 1 : 0;
		integrated_gameplay_trace[(calls % 24) * 15 + 13] = state.input.actions_held[ZOO_ACTION_RIGHT] ? 1 : 0;
		integrated_gameplay_trace[(calls % 24) * 15 + 14] = state.input.actions_held[ZOO_ACTION_LEFT] ? 1 : 0;
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 10 - state.error_value;
		}
		calls++;
		if (integrated_board_change_checkpoint_failed) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 4;
		}
		if (integrated_board_change_checkpoint_captured) {
			integrated_board_change_checkpoint_captured = false;
			transition_count++;
			last_board = state.world.info.current_board;
			last_game_state = state.game_state;
			last_paused = state.game_paused;
		} else if (tick_result != RETURN_IMMEDIATE
				&& (state.world.info.current_board != last_board
				|| state.game_state != last_game_state
				|| state.game_paused != last_paused)) {
			last_board = state.world.info.current_board;
			last_game_state = state.game_state;
			last_paused = state.game_paused;
			transition_count++;
			if (!capture_integrated_gameplay_snapshot(
					integrated_gameplay_snapshot_count_value, 'T', transition_count)) {
				force_elapsed_for_tick_check = false;
				integrated_schedule_enabled = false;
				return 4;
			}
			if (integrated_pause_transition_checkpoint_pending
					&& transition_count == 3 && state.game_paused) {
				integrated_pause_transition_checkpoint_pending = false;
			}
		}
		if (state.current_stat_tick == 0 && state.current_tick != previous_tick) {
			cycles++;
			if (integrated_cycle_clock_enabled)
				integrated_fixed_clock_hsecs = (int16_t) ((cycles * 12) % 6000);
			if (cycles == cycle_limit) state.game_play_exit_requested = true;
			if (cycles > cycle_limit) {
				force_elapsed_for_tick_check = false;
				integrated_schedule_enabled = false;
				return 11;
			}
			if (!capture_integrated_gameplay_snapshot(
					integrated_gameplay_snapshot_count_value, 'C', cycles)) {
				force_elapsed_for_tick_check = false;
				integrated_schedule_enabled = false;
				return 4;
			}
		}
	}
	force_elapsed_for_tick_check = false;
	integrated_schedule_enabled = false;
	if (cycles != cycle_limit) return 100 + cycles;
	if (state.game_paused) return 200;
	if (integrated_normalized_input_tape != NULL
			&& integrated_normalized_input_tape_position != integrated_normalized_input_tape_length)
		return 24;
	integrated_gameplay_sound_capture_enabled = false;
	integrated_gameplay_cell_capture_enabled = false;
	/* Match GamePlayLoop's non-gameplay cleanup before taking the snapshot. */
	zoo_sound_clear_queue(&state.sound);
	state.board.tiles[state.board.stats[0].x][state.board.stats[0].y].element = ZOO_E_PLAYER;
	state.board.tiles[state.board.stats[0].x][state.board.stats[0].y].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.sound.block_queueing = false;
	state.input.key_pressed = integrated_gameplay_post_loop_key_pressed;
	save_handle = zoo_io_open_file_mem(integrated_gameplay_save_bytes,
		INTEGRATED_GAMEPLAY_SAVE_CAPACITY, MODE_WRITE);
	integrated_gameplay_save_result_value = zoo_world_save(&state, &save_handle);
	integrated_gameplay_save_length_value = (int32_t) save_handle.func_tell(&save_handle);
	if (integrated_gameplay_save_length_value >= INTEGRATED_GAMEPLAY_SAVE_CAPACITY) {
		integrated_gameplay_save_overflow_value = 1;
		return 12;
	}
	if (integrated_gameplay_save_result_value != 0) return 13;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'T', 2)) return 4;
	return cycles == cycle_limit ? 0 : 3;
}

int32_t run_integrated_title_lifecycle_checks(void) {
	int32_t calls;
	int16_t previous_tick;
	zoo_tick_retval tick_result;
	int result;

	integrated_gameplay_capture_reset();
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	state.input.repeat_start = 4;
	state.input.repeat_end = 6;
	state.tick_speed = 0;
	zoo_world_create(&state);
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	zoo_game_start(&state, GS_TITLE);
	integrated_schedule_enabled = true;
	integrated_configured_tick_speed = 0;
	integrated_elapsed_nanos = 0;
	integrated_fixed_clock_enabled = false;
	force_elapsed_for_tick_check = true;
	zoo_input_key_press(&state.input, 'P');
	zoo_input_key_press(&state.input, 'X');
	for (calls = 0; calls < 1000 && state.input.key_pressed != 'P'; calls++) {
		tick_result = zoo_tick(&state);
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 20 - state.error_value;
		}
	}
	if (state.input.key_pressed != 'P') {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 21;
	}
	result = zoo_world_play(&state);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 22 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(0, 'P', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 23;
	}
	previous_tick = state.current_tick;
	for (calls = 0; calls < 1000; calls++) {
		tick_result = zoo_tick(&state);
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 24 - state.error_value;
		}
		if (state.current_stat_tick == 0 && state.current_tick != previous_tick) break;
	}
	if (calls == 1000) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 25;
	}
	state.game_play_exit_requested = true;
	if (!capture_integrated_gameplay_snapshot(1, 'C', 1)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 26;
	}
	result = zoo_world_return_title(&state);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 27 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(2, 'T', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 28;
	}
	force_elapsed_for_tick_check = false;
	integrated_schedule_enabled = false;
	return 0;
}

int32_t run_integrated_title_saved_world_play_checks(int32_t world_length) {
	int32_t calls;
	int16_t previous_tick;
	zoo_tick_retval tick_result;
	zoo_io_handle title_handle;
	int result;

	if (world_length <= 0 || world_length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 30;
	integrated_gameplay_capture_reset();
	memset(&integrated_title_world_io, 0, sizeof(integrated_title_world_io));
	integrated_title_world_length = world_length;
	integrated_title_world_io.read_only = true;
	integrated_title_world_io.func_open_file = integrated_title_world_open_file;
	state.d_video = &video;
	state.d_io = &integrated_title_world_io;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	state.input.repeat_start = 4;
	state.input.repeat_end = 6;
	state.tick_speed = 0;
	zoo_world_create(&state);
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	title_handle = zoo_io_open_file_mem(integrated_demo_world_bytes,
		(size_t) world_length, MODE_READ);
	result = zoo_world_load(&state, &title_handle, true);
	if (title_handle.func_close != NULL) title_handle.func_close(&title_handle);
	if (result != 0 || state.error_value != 0) return 31 - (result != 0 ? result : state.error_value);
	zoo_io_pstring_assign_pascal(state.world.info.name, sizeof(state.world.info.name) - 1,
		state.world.info.io_name_data, &state.world.info.io_name_length,
		&state.world.info.io_name_valid, "DEMO", 4);
	state.world.info.is_save = true;
	state.world.info.health = 1;
	state.world.info.ammo = 99;
	zoo_board_change(&state, 0);
	if (state.error_value != 0) return 32 - state.error_value;
	zoo_game_start(&state, GS_TITLE);
	integrated_schedule_enabled = true;
	integrated_configured_tick_speed = 0;
	integrated_elapsed_nanos = 0;
	integrated_fixed_clock_enabled = false;
	force_elapsed_for_tick_check = true;
	zoo_input_key_press(&state.input, 'P');
	zoo_input_key_press(&state.input, 'X');
	for (calls = 0; calls < 1000 && state.input.key_pressed != 'P'; calls++) {
		tick_result = zoo_tick(&state);
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 33 - state.error_value;
		}
	}
	if (state.input.key_pressed != 'P') {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 34;
	}
	result = zoo_world_play(&state);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 35 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(0, 'P', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 36;
	}
	previous_tick = state.current_tick;
	for (calls = 0; calls < 1000; calls++) {
		tick_result = zoo_tick(&state);
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 37 - state.error_value;
		}
		if (state.current_stat_tick == 0 && state.current_tick != previous_tick) break;
	}
	if (calls == 1000) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 38;
	}
	state.game_play_exit_requested = true;
	if (!capture_integrated_gameplay_snapshot(1, 'C', 1)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 39;
	}
	result = zoo_world_return_title(&state);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 40 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(2, 'T', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 41;
	}
	force_elapsed_for_tick_check = false;
	integrated_schedule_enabled = false;
	return 0;
}

int32_t run_integrated_end_play_checks(void) {
	static const uint8_t answer_keys[3] = { 'Y', 'N', 27 };
	int32_t row = 0;
	int paused, answer;
	memset(integrated_end_play_output, 0, sizeof(integrated_end_play_output));
	for (paused = 0; paused <= 1; paused++) {
		for (answer = 0; answer < 3; answer++) {
			memset(&state, 0, sizeof(state));
			memset(&video, 0, sizeof(video));
			state.d_video = &video;
			state.func_draw_sidebar = test_sidebar_update;
			state.func_random = integrated_gameplay_random;
			state.random_seed = 0;
			zoo_world_create(&state);
			zoo_game_start(&state, GS_PLAY);
			state.world.info.health = 37;
			state.game_paused = paused != 0;
			state.input.key_pressed = 'Q';
			zoo_game_resolve_end_play(&state, answer_keys[answer] == 'Y', paused != 0);
			integrated_end_play_output[row][0] = row + 1;
			integrated_end_play_output[row][1] = state.game_play_exit_requested ? 1 : 0;
			integrated_end_play_output[row][2] = state.input.key_pressed;
			integrated_end_play_output[row][3] = state.game_paused ? 1 : 0;
			integrated_end_play_output[row][4] = state.world.info.health;
			row++;
		}
	}
	memset(&state, 0, sizeof(state));
	memset(&video, 0, sizeof(video));
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	zoo_world_create(&state);
	zoo_game_start(&state, GS_PLAY);
	state.world.info.health = 0;
	state.input.key_pressed = 27;
	zoo_game_resolve_end_play(&state, true, false);
	integrated_end_play_output[row][0] = row + 1;
	integrated_end_play_output[row][1] = state.game_play_exit_requested ? 1 : 0;
	integrated_end_play_output[row][2] = state.input.key_pressed;
	integrated_end_play_output[row][3] = state.game_paused ? 1 : 0;
	integrated_end_play_output[row][4] = state.world.info.health;
	return 0;
}

uint32_t integrated_end_play_output_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_end_play_output;
}

int32_t integrated_end_play_output_rows(void) {
	return 7;
}

int32_t run_integrated_title_quit_checks(void) {
	int32_t row;
	memset(integrated_title_quit_output, 0, sizeof(integrated_title_quit_output));
	for (row = 0; row < 3; row++) {
		memset(&state, 0, sizeof(state));
		memset(&video, 0, sizeof(video));
		state.d_video = &video;
		state.func_draw_sidebar = test_sidebar_update;
		state.func_random = integrated_gameplay_random;
		state.random_seed = 0;
		zoo_world_create(&state);
		zoo_game_start(&state, GS_TITLE);
		zoo_game_resolve_title_quit(&state, row == 0);
		integrated_title_quit_output[row][0] = row + 1;
		integrated_title_quit_output[row][1] = state.game_title_exit_requested ? 1 : 0;
	}
	return 0;
}

uint32_t integrated_title_quit_output_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_title_quit_output;
}

int32_t integrated_title_quit_output_rows(void) {
	return 3;
}

static int32_t run_integrated_title_restore_replay(int32_t save_length, bool capture_cancel) {
	int32_t calls;
	int16_t previous_tick;
	zoo_tick_retval tick_result;
	zoo_io_handle save_handle;
	int32_t checkpoint_index;
	int result;

	if (save_length <= 0 || save_length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 30;
	integrated_gameplay_capture_reset();
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	state.input.repeat_start = 4;
	state.input.repeat_end = 6;
	state.tick_speed = 0;
	zoo_world_create(&state);
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	zoo_game_start(&state, GS_TITLE);
	state.input.key_pressed = '\r'; /* TextWindowSelect accepts the chosen .SAV row with Enter. */
	integrated_schedule_enabled = true;
	integrated_configured_tick_speed = 0;
	integrated_elapsed_nanos = 0;
	integrated_fixed_clock_enabled = false;
	force_elapsed_for_tick_check = true;
	checkpoint_index = 0;
	if (capture_cancel) {
		/* The Pascal title monitor is already displaying the loaded first board
		   when the file selector is cancelled. Seed that same live state from
		   the Pascal-generated save, then restore the pre-save menu counters. */
		save_handle = zoo_io_open_file_mem(integrated_demo_world_bytes, (size_t) save_length, MODE_READ);
		result = zoo_world_load(&state, &save_handle, false);
		if (save_handle.func_close != NULL) save_handle.func_close(&save_handle);
		if (result != 0 || state.error_value != 0) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 38 - (result != 0 ? result : state.error_value);
		}
		state.world.info.health = 1;
		state.world.info.ammo = 99;
		/* TextWindowSelect consumes Escape; the menu itself is a presentation
		   adapter, so no Escape key remains in engine input at checkpoint X. */
		state.input.key_pressed = 0;
		/* GameTitleLoop has completed one monitor cycle before it opens the
		   selector. Advance the shared engine through that same zero-speed cycle. */
		previous_tick = state.current_tick;
		for (calls = 0; calls < 1000; calls++) {
			tick_result = zoo_tick(&state);
			if (tick_result == ERROR) {
				force_elapsed_for_tick_check = false;
				integrated_schedule_enabled = false;
				return 40 - state.error_value;
			}
			if (state.current_stat_tick == 0 && state.current_tick != previous_tick) break;
		}
		if (calls == 1000) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 41;
		}
		/* The title menu's R handler exits its monitor GamePlayLoop before the
		   selector is displayed; that platform-owned handoff leaves this flag set. */
		state.game_play_exit_requested = true;
		if (!capture_integrated_gameplay_snapshot(checkpoint_index++, 'X', 0)) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 42;
		}
		/* Retrying the title-menu R route starts a second monitor GamePlayLoop,
		   consuming its own Random(100) and logical-clock poll before the restored
		   player loop starts. */
		state.current_tick = integrated_gameplay_random(&state, 100);
		state.current_stat_tick = state.board.stat_count + 1;
		state.game_play_exit_requested = false;
		integrated_elapsed_nanos = 50000000;
		previous_tick = state.current_tick;
		for (calls = 0; calls < 1000; calls++) {
			tick_result = zoo_tick(&state);
			if (tick_result == ERROR) {
				force_elapsed_for_tick_check = false;
				integrated_schedule_enabled = false;
				return 43 - state.error_value;
			}
			if (state.current_stat_tick == 0 && state.current_tick != previous_tick) break;
		}
		if (calls == 1000) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 44;
		}
		state.game_play_exit_requested = true;
	}
	save_handle = zoo_io_open_file_mem(integrated_demo_world_bytes, (size_t) save_length, MODE_READ);
	state.input.key_pressed = '\r'; /* The retried TextWindowSelect accepts the chosen .SAV row. */
	result = zoo_world_restore(&state, &save_handle);
	if (save_handle.func_close != NULL) save_handle.func_close(&save_handle);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 31 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(checkpoint_index++, 'P', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 32;
	}
	/* The compiled Pascal title-R replay polls 5 then 10 hsecs before its first
	   paused cycle boundary. Seed the production clock tape so its two polls
	   reach the same logical boundary independent of host polling cadence. */
	/* A cancelled/retried title selection has already consumed one monitor
	   cycle, so continue its clock tape rather than restarting at 5 hsecs. */
	integrated_elapsed_nanos = capture_cancel ? 110000000 : 50000000;
	previous_tick = state.current_tick;
	for (calls = 0; calls < 1000; calls++) {
		tick_result = zoo_tick(&state);
		if (tick_result == ERROR) {
			force_elapsed_for_tick_check = false;
			integrated_schedule_enabled = false;
			return 33 - state.error_value;
		}
		if (state.current_stat_tick == 0 && state.current_tick != previous_tick) break;
	}
	if (calls == 1000) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 34;
	}
	state.game_play_exit_requested = true;
	if (!capture_integrated_gameplay_snapshot(checkpoint_index++, 'C', 1)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 35;
	}
	result = zoo_world_return_title(&state);
	if (result != 0 || state.error_value != 0) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 36 - (result != 0 ? result : state.error_value);
	}
	if (!capture_integrated_gameplay_snapshot(checkpoint_index, 'T', 0)) {
		force_elapsed_for_tick_check = false;
		integrated_schedule_enabled = false;
		return 37;
	}
	force_elapsed_for_tick_check = false;
	integrated_schedule_enabled = false;
	return 0;
}

int32_t run_integrated_title_restore_checks(int32_t save_length) {
	return run_integrated_title_restore_replay(save_length, false);
}

int32_t run_integrated_title_restore_retry_checks(int32_t save_length) {
	return run_integrated_title_restore_replay(save_length, true);
}

static void integrated_title_restore_failure_append(int32_t value) {
	if (integrated_title_restore_failure_field_count_value < INTEGRATED_TITLE_RESTORE_FAILURE_FIELD_CAPACITY) {
		integrated_title_restore_failure_fields[integrated_title_restore_failure_field_count_value++] = value;
	}
}

int32_t run_integrated_title_restore_failure_checks(int32_t save_length) {
	zoo_io_handle save_handle;
	int result;
	int16_t ix, iy;
	zoo_stat *player;

	if (save_length <= 0 || save_length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 38;
	integrated_gameplay_capture_reset();
	memset(integrated_title_restore_failure_fields, 0,
		sizeof(integrated_title_restore_failure_fields));
	integrated_title_restore_failure_field_count_value = 0;
	integrated_title_restore_failure_error_value = 0;
	state.d_video = &video;
	state.func_draw_sidebar = test_sidebar_update;
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	zoo_world_create(&state);
	state.func_random = integrated_gameplay_random;
	state.random_seed = 0;
	zoo_game_start(&state, GS_TITLE);
	state.input.key_pressed = '\r';
	save_handle = zoo_io_open_file_mem(integrated_demo_world_bytes, (size_t) save_length, MODE_READ);
	result = zoo_world_restore(&state, &save_handle);
	if (save_handle.func_close != NULL) save_handle.func_close(&save_handle);
	integrated_title_restore_failure_error_value = result;

	/* Field 0 corresponds to Pascal GameWorldLoad's Boolean result. */
	integrated_title_restore_failure_append(result == 0 ? 1 : 0);
	integrated_title_restore_failure_append(state.world.board_count);
	integrated_title_restore_failure_append(state.world.board_len[0]);
	integrated_title_restore_failure_append(state.return_board_id);
	integrated_title_restore_failure_append(state.world.info.current_board);
	integrated_title_restore_failure_append(state.world.info.ammo);
	integrated_title_restore_failure_append(state.world.info.gems);
	integrated_title_restore_failure_append(state.world.info.health);
	integrated_title_restore_failure_append(state.world.info.torches);
	integrated_title_restore_failure_append(state.world.info.torch_ticks);
	integrated_title_restore_failure_append(state.world.info.energizer_ticks);
	integrated_title_restore_failure_append(state.world.info.score);
	integrated_title_restore_failure_append(state.world.info.board_time_sec);
	integrated_title_restore_failure_append(state.world.info.board_time_hsec);
	integrated_title_restore_failure_append(state.world.info.is_save ? 1 : 0);
	integrated_title_restore_failure_append(state.board.stat_count);
	integrated_title_restore_failure_append(state.input.key_pressed);
	player = &state.board.stats[0];
	integrated_title_restore_failure_append(player->x);
	integrated_title_restore_failure_append(player->y);
	integrated_title_restore_failure_append(player->step_x);
	integrated_title_restore_failure_append(player->step_y);
	integrated_title_restore_failure_append(player->cycle);
	integrated_title_restore_failure_append(player->p1);
	integrated_title_restore_failure_append(player->p2);
	integrated_title_restore_failure_append(player->p3);
	integrated_title_restore_failure_append(player->follower);
	integrated_title_restore_failure_append(player->leader);
	integrated_title_restore_failure_append(player->under.element);
	integrated_title_restore_failure_append(player->under.color);
	integrated_title_restore_failure_append(player->data_pos);
	integrated_title_restore_failure_append(player->data_len);
	for (ix = 0; ix < 7; ix++)
		integrated_title_restore_failure_append(state.world.info.keys[ix] ? 1 : 0);
	for (ix = 0; ix <= ZOO_BOARD_WIDTH + 1; ix++) {
		for (iy = 0; iy <= ZOO_BOARD_HEIGHT + 1; iy++) {
			integrated_title_restore_failure_append(state.board.tiles[ix][iy].element);
			integrated_title_restore_failure_append(state.board.tiles[ix][iy].color);
		}
	}
	return 0;
}

uint32_t integrated_title_restore_failure_fields_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_title_restore_failure_fields;
}

int32_t integrated_title_restore_failure_field_count(void) {
	return integrated_title_restore_failure_field_count_value;
}

int32_t integrated_title_restore_failure_error(void) {
	return integrated_title_restore_failure_error_value;
}

int32_t run_integrated_gameplay_checks(void) {
	static const char script[] = "#GIVE GEMS 1\r#IDLE\r";
	static const char script2[] = "#GIVE AMMO 1\r#IDLE\r";
	int16_t stat_id;
	int result;

	integrated_gameplay_capture_reset();
	/* Match the Pascal fixture's E_PLAYER marker before its loaded snapshot. */
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.gems = 0;
	state.world.info.ammo = 0;
	state.world.info.score = 0;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.tiles[6][5].element = ZOO_E_BOULDER;
	state.board.tiles[6][5].color = 0x0E;
	state.board.tiles[7][5].element = ZOO_E_EMPTY;
	state.board.tiles[7][5].color = 0x70;
	zoo_stat_add(&state, 20, 5, ZOO_E_OBJECT, 0x0A, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 20, 5);
	if (stat_id != 1) return 1;
	state.board.stats[stat_id].data = (char *) script;
	state.board.stats[stat_id].data_len = (int16_t) (sizeof(script) - 1);
	state.board.stats[stat_id].data_pos = 0;
	zoo_stat_add(&state, 25, 5, ZOO_E_OBJECT, 0x0A, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 25, 5);
	if (stat_id != 2) return 2;
	state.board.stats[stat_id].data = (char *) script2;
	state.board.stats[stat_id].data_len = (int16_t) (sizeof(script2) - 1);
	state.board.stats[stat_id].data_pos = 0;
	result = integrated_gameplay_run_ticks("66", 2, NULL, 0, 6, false, false, false, false);
	if (result != 0) return result;
	return state.world.info.ammo == 1 && state.world.info.gems == 1 ? 0 : 20;
}

uint32_t integrated_demo_world_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_demo_world_bytes;
}

int32_t integrated_demo_world_bytes_capacity(void) {
	return INTEGRATED_GAMEPLAY_SAVE_CAPACITY;
}

int32_t run_integrated_demo_gameplay_checks(int32_t demo_world_length) {
	zoo_io_handle demo_handle;
	int result;
	if (demo_world_length <= 0 || demo_world_length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 30;
	integrated_gameplay_capture_reset();
	zoo_world_create(&state);
	demo_handle = zoo_io_open_file_mem(integrated_demo_world_bytes,
		(size_t) demo_world_length, MODE_READ);
	result = zoo_world_load(&state, &demo_handle, false);
	if (result != 0) return 31;
	zoo_board_change(&state, state.return_board_id);
	if (state.error_value != 0) return 32;
	zoo_board_enter(&state);
	state.game_state = GS_PLAY;
	state.game_paused = true;
	state.game_play_exit_requested = false;
	result = integrated_gameplay_run_ticks("666666", 6, NULL, 0, 6, false, false, false, false);
	return result;
}

int32_t run_integrated_wall_pusher_duplicator_checks(void) {
	static const char duplicated_object_script[] = "#GIVE GEMS 1\r#IDLE\r";
	int16_t stat_id;
	int result;

	integrated_gameplay_capture_reset();
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.gems = 0;
	state.world.info.ammo = 0;
	state.world.info.score = 0;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.tiles[6][5].element = ZOO_E_EMPTY;
	state.board.tiles[6][5].color = 0x70;
	state.board.tiles[7][5].element = ZOO_E_BOULDER;
	state.board.tiles[7][5].color = 0x0E;
	state.board.tiles[8][5].element = ZOO_E_EMPTY;
	state.board.tiles[8][5].color = 0x70;
	state.board.tiles[9][5].element = ZOO_E_SOLID;
	state.board.tiles[9][5].color = 0x0F;

	/* Preserve the original stat order: object, two pushers, duplicator. */
	zoo_stat_add(&state, 21, 15, ZOO_E_OBJECT, 0x0A, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 21, 15);
	if (stat_id != 1) return 40;
	state.board.stats[stat_id].data = (char *) duplicated_object_script;
	state.board.stats[stat_id].data_len = (int16_t) (sizeof(duplicated_object_script) - 1);
	state.board.stats[stat_id].data_pos = 0;

	zoo_stat_add(&state, 10, 10, ZOO_E_PUSHER, 0x0F, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 10, 10);
	if (stat_id != 2) return 41;
	state.board.stats[stat_id].step_x = 1;
	zoo_stat_add(&state, 11, 10, ZOO_E_PUSHER, 0x0F, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 11, 10);
	if (stat_id != 3) return 42;
	state.board.stats[stat_id].step_x = 1;
	state.board.tiles[12][10].element = ZOO_E_BOULDER;
	state.board.tiles[12][10].color = 0x0E;
	state.board.tiles[13][10].element = ZOO_E_EMPTY;
	state.board.tiles[13][10].color = 0x70;
	state.board.tiles[14][10].element = ZOO_E_SOLID;
	state.board.tiles[14][10].color = 0x0F;

	zoo_stat_add(&state, 20, 15, ZOO_E_DUPLICATOR, 0x0E, 1, &zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 20, 15);
	if (stat_id != 4) return 43;
	state.board.stats[stat_id].step_x = 1;
	state.board.stats[stat_id].p1 = 5;
	state.board.stats[stat_id].p2 = 2;
	state.board.tiles[19][15].element = ZOO_E_EMPTY;
	state.board.tiles[19][15].color = 0x70;

	result = integrated_gameplay_run_ticks("666666", 6, NULL, 0, 6, false, false, false, false);
	return result;
}

int32_t run_integrated_combat_key_pickup_checks(void) {
	int16_t lion_id;
	int result;

	integrated_gameplay_capture_reset();
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.ammo = 1;
	state.world.info.gems = 0;
	state.world.info.score = 0;
	state.world.info.keys[3] = false;
	state.world.info.io_key_bytes[3] = 0;
	state.board.info.max_shots = 2;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.tiles[6][5].element = ZOO_E_KEY;
	state.board.tiles[6][5].color = 4;
	state.board.tiles[8][5].element = ZOO_E_GEM;
	state.board.tiles[8][5].color = zoo_element_defs[ZOO_E_GEM].color;
	zoo_stat_add(&state, 7, 5, ZOO_E_LION, zoo_element_defs[ZOO_E_LION].color, 0,
		&zoo_stat_template_default);
	lion_id = zoo_stat_get_id(&state, 7, 5);
	if (lion_id != 1) return 40;
	state.board.stats[lion_id].cycle = 0;
	state.board.stats[lion_id].under.element = ZOO_E_EMPTY;
	state.board.stats[lion_id].under.color = 0;
	state.board.tiles[9][5].element = ZOO_E_DOOR;
	state.board.tiles[9][5].color = 0x4F;
	state.board.tiles[10][5].element = ZOO_E_AMMO;
	state.board.tiles[10][5].color = zoo_element_defs[ZOO_E_AMMO].color;

	result = integrated_gameplay_run_ticks(NULL, 0, "RFRRRRR", 7, 6, false, false, false, false);
	if (result != 0) return result;
	if (state.board.stats[0].x != 10 || state.board.stats[0].y != 5) return 41;
	if (state.world.info.ammo != 5) return 100 + state.world.info.ammo;
	if (state.world.info.gems != 1) return 200 + state.world.info.gems;
	if (state.world.info.health != 101) return 300 + state.world.info.health;
	if (state.world.info.keys[3]) return 400;
	if (state.world.info.score != 10) return 500 + state.world.info.score;
	if (state.board.stat_count != 1) return 600 + state.board.stat_count;
	if (state.board.tiles[6][5].element != ZOO_E_EMPTY
			|| state.board.tiles[7][5].element != ZOO_E_EMPTY
			|| state.board.tiles[8][5].element != ZOO_E_EMPTY
			|| state.board.tiles[9][5].element != ZOO_E_EMPTY
			|| state.board.tiles[10][5].element != ZOO_E_PLAYER) return 43;
	return 0;
}

int32_t run_integrated_script_stat_order_checks(void) {
	static char first_script[] = "#SET FIRST\r#GIVE GEMS 1\r#END\r";
	static char second_script[] = "#IF FIRST #GIVE AMMO 1\r#END\r";
	char *first_data;
	char *second_data;
	uint32_t first_offset;
	uint32_t second_offset;
	int16_t first_id;
	int16_t second_id;
	int result;

	integrated_gameplay_capture_reset();
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.gems = 0;
	state.world.info.ammo = 0;
	state.world.info.score = 0;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;

	/* Insert the farther-right object first; execution must retain stat order. */
	zoo_stat_add(&state, 20, 5, ZOO_E_OBJECT, 0x0A,
		1, &zoo_stat_template_default);
	first_id = zoo_stat_get_id(&state, 20, 5);
	if (first_id != 1) return 40;
	first_data = zoo_board_data_heap_alloc(&state.board, (uint32_t) (sizeof(first_script) - 1), &first_offset);
	if (first_data == NULL) return 42;
	memcpy(first_data, first_script, sizeof(first_script) - 1);
	state.board.stats[first_id].data = first_data;
	state.board.stats[first_id].data_len = (int16_t) (sizeof(first_script) - 1);
	state.board.stats[first_id].data_pos = 0;
	state.board.stats[first_id].data_in_heap = true;
	state.board.stats[first_id].data_heap_offset = first_offset;

	zoo_stat_add(&state, 10, 5, ZOO_E_OBJECT, 0x0A,
		1, &zoo_stat_template_default);
	second_id = zoo_stat_get_id(&state, 10, 5);
	if (second_id != 2) return 41;
	second_data = zoo_board_data_heap_alloc(&state.board, (uint32_t) (sizeof(second_script) - 1), &second_offset);
	if (second_data == NULL) return 43;
	memcpy(second_data, second_script, sizeof(second_script) - 1);
	state.board.stats[second_id].data = second_data;
	state.board.stats[second_id].data_len = (int16_t) (sizeof(second_script) - 1);
	state.board.stats[second_id].data_pos = 0;
	state.board.stats[second_id].data_in_heap = true;
	state.board.stats[second_id].data_heap_offset = second_offset;

	result = integrated_gameplay_run_ticks("66", 2, NULL, 0, 6, false, false, false, false);
	if (result != 0) return result;
	return 0;
}

int32_t run_integrated_board_neighbor_passage_checks(void) {
	int16_t stat_id, i, random_index;
	uint16_t swap;
	int result;

	integrated_gameplay_capture_reset();
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		state.transition_order[i] = (uint16_t) i;
	}
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		random_index = integrated_gameplay_random(&state, ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT);
		swap = state.transition_order[random_index];
		state.transition_order[random_index] = state.transition_order[i];
		state.transition_order[i] = swap;
	}
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	state.world.board_count = 2;
	state.world.board_data[1] = NULL;
	state.world.board_data[2] = NULL;
	state.world.board_len[1] = 0;
	state.world.board_len[2] = 0;
	state.world.board_external[1] = false;
	state.world.board_external[2] = false;
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.ammo = 0;
	state.world.info.gems = 0;
	state.world.info.score = 0;
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.tiles[1][5].element = ZOO_E_EMPTY;
	state.board.tiles[1][5].color = 0;
	state.board.stats[0].x = 2;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[2][5].element = ZOO_E_PLAYER;
	state.board.tiles[2][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.info.neighbor_boards[2] = 1;
	if (zoo_board_close(&state) != 0) return 40;

	state.world.info.current_board = 1;
	zoo_board_create(&state);
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.stats[0].x = 60;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[60][5].element = ZOO_E_PLAYER;
	state.board.tiles[60][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	zoo_stat_add(&state, 59, 5, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 59, 5);
	if (stat_id != 1) return 41;
	state.board.stats[stat_id].p3 = 2;
	if (zoo_board_close(&state) != 0) return 42;

	state.world.info.current_board = 2;
	zoo_board_create(&state);
	state.board.tiles[23][17].element = ZOO_E_EMPTY;
	state.board.tiles[23][17].color = 0;
	zoo_stat_add(&state, 20, 15, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	zoo_stat_add(&state, 22, 17, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	if (zoo_board_close(&state) != 0) return 43;
	zoo_board_change(&state, 0);
	if (state.error_value != 0) return 44;
	state.game_state = GS_PLAY;
	/* The pause cursor can redraw its unchanged glyph on extra logical polls.
	   The replay records actual cell changes and reports these idempotent writes separately. */
	integrated_gameplay_cell_coalesce_idempotent_writes = true;
	result = integrated_gameplay_run_ticks(NULL, 0, "LLLR", 4, 6, true, true, true, false);
	if (result != 0) return result;
	if (state.world.info.current_board != 2
			|| state.board.stats[0].x != 23 || state.board.stats[0].y != 17
			|| state.board.stat_count != 2 || state.game_paused) return 45;
	return 0;
}

int32_t run_integrated_dark_room_torch_energizer_timers_death_checks(void) {
	int result;

	integrated_gameplay_capture_reset();
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	/* The Pascal startup transition shuffle advances RandSeed 0 before the loop starts. */
	state.random_seed = (int32_t) 0x240F47C4u;
	state.world.info.health = 10;
	state.world.info.torches = 0;
	state.world.info.torch_ticks = 0;
	state.world.info.energizer_ticks = 0;
	state.world.info.current_board = 0;
	state.board.info.is_dark = true;
	state.board.info.io_is_dark_byte = 1;
	state.board.info.time_limit_sec = 10;
	state.board.info.start_player_x = 5;
	state.board.info.start_player_y = 5;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.tiles[6][5].element = ZOO_E_TORCH;
	state.board.tiles[6][5].color = zoo_element_defs[ZOO_E_TORCH].color;
	state.board.tiles[7][5].element = ZOO_E_ENERGIZER;
	state.board.tiles[7][5].color = zoo_element_defs[ZOO_E_ENERGIZER].color;
	state.board.tiles[8][5].element = ZOO_E_SOLID;
	state.board.tiles[8][5].color = zoo_element_defs[ZOO_E_SOLID].color;
	state.board.tiles[40][20].element = ZOO_E_EMPTY;
	state.board.tiles[40][20].color = 0x70;

	/* Report only effective cell changes; identical redraws remain counted separately. */
	integrated_gameplay_cell_coalesce_idempotent_writes = true;
	result = integrated_gameplay_run_ticks(NULL, 0, "RTRN", 4, 120, true, false, false, true);
	return result;
}

int32_t run_integrated_pause_scheduler_checks(void) {
	int result;

	integrated_gameplay_capture_reset();
	/* The Pascal fixture performs ZZT's startup transition-table shuffle before
	   WorldCreate; use its measured post-shuffle seed for comparable Random calls. */
	state.random_seed = (int32_t) 0x240F47C4u;
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.current_board = 0;
	state.board.info.start_player_x = 5;
	state.board.info.start_player_y = 5;
	state.board.stats[0].x = 5;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.tiles[5][5].element = ZOO_E_PLAYER;
	state.board.tiles[5][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	/* Pause blinking can issue cadence-dependent repeated writes; compare the
	   ordered effective screen changes and report identical redraws separately. */
	integrated_gameplay_cell_coalesce_idempotent_writes = true;
	integrated_configured_tick_speed = 6;
	result = integrated_gameplay_run_ticks(NULL, 0, "RPNNRNNN", 8, 5, true, false, false, false);
	if (result != 0) return result;
	if (state.tick_speed != 6 || state.tick_duration != 12
			|| state.game_paused || state.board.stats[0].x != 7 || state.board.stats[0].y != 5) return 46;
	return 0;
}

int32_t run_integrated_save_restore_world_reload_checks(void) {
	int16_t stat_id, i, random_index;
	uint16_t swap;
	int result;

	integrated_gameplay_capture_reset();
	/* Match the Pascal startup transition-table shuffle used by F05. */
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		state.transition_order[i] = (uint16_t) i;
	}
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		random_index = integrated_gameplay_random(&state, ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT);
		swap = state.transition_order[random_index];
		state.transition_order[random_index] = state.transition_order[i];
		state.transition_order[i] = swap;
	}
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	state.world.board_count = 2;
	state.world.board_data[1] = NULL;
	state.world.board_data[2] = NULL;
	state.world.board_len[1] = 0;
	state.world.board_len[2] = 0;
	state.world.board_external[1] = false;
	state.world.board_external[2] = false;
	zoo_board_create(&state);
	state.world.info.health = 100;
	state.world.info.ammo = 0;
	state.world.info.gems = 0;
	state.world.info.score = 0;
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.tiles[1][5].element = ZOO_E_EMPTY;
	state.board.tiles[1][5].color = 0;
	state.board.stats[0].x = 2;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].cycle = 1;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[2][5].element = ZOO_E_PLAYER;
	state.board.tiles[2][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	state.board.info.neighbor_boards[2] = 1;
	if (zoo_board_close(&state) != 0) return 40;

	state.world.info.current_board = 1;
	zoo_board_create(&state);
	state.board.tiles[30][12].element = ZOO_E_EMPTY;
	state.board.tiles[30][12].color = 0;
	state.board.stats[0].x = 60;
	state.board.stats[0].y = 5;
	state.board.stats[0].step_x = 0;
	state.board.stats[0].step_y = 0;
	state.board.stats[0].under.element = ZOO_E_EMPTY;
	state.board.stats[0].under.color = 0;
	state.board.tiles[60][5].element = ZOO_E_PLAYER;
	state.board.tiles[60][5].color = zoo_element_defs[ZOO_E_PLAYER].color;
	zoo_stat_add(&state, 59, 5, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	stat_id = zoo_stat_get_id(&state, 59, 5);
	if (stat_id != 1) return 41;
	state.board.stats[stat_id].p3 = 2;
	if (zoo_board_close(&state) != 0) return 42;

	state.world.info.current_board = 2;
	zoo_board_create(&state);
	state.board.tiles[23][17].element = ZOO_E_EMPTY;
	state.board.tiles[23][17].color = 0;
	zoo_stat_add(&state, 20, 15, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	zoo_stat_add(&state, 22, 17, ZOO_E_PASSAGE, 0x2A, 0,
		&zoo_stat_template_default);
	if (zoo_board_close(&state) != 0) return 43;
	zoo_board_change(&state, 0);
	if (state.error_value != 0) return 44;
	state.game_state = GS_PLAY;
	integrated_gameplay_cell_coalesce_idempotent_writes = true;
	result = integrated_gameplay_run_ticks(NULL, 0, "LLLR", 4, 6, true, true, true, false);
	if (result != 0) return result;
	if (state.world.info.current_board != 2
			|| state.board.stats[0].x != 23 || state.board.stats[0].y != 17
			|| state.board.stat_count != 2 || state.game_paused) return 45;
	return 0;
}

int32_t run_integrated_editor_brd_roundtrip_checks(void) {
	int16_t i, random_index;
	uint16_t swap;
	size_t body_length = 0;
	zoo_editor_brd_transfer_outcome outcome = ZOO_EDITOR_BRD_TRANSFER_IMPORTED;
	int result;

	integrated_gameplay_capture_reset();
	memset(integrated_editor_brd_bytes, 0, sizeof(integrated_editor_brd_bytes));
	integrated_editor_brd_length_value = 0;
	/* Match Pascal startup: GenerateTransitionTable runs before WorldCreate. */
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		state.transition_order[i] = (uint16_t) i;
	}
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		random_index = integrated_gameplay_random(&state, ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT);
		swap = state.transition_order[random_index];
		state.transition_order[random_index] = state.transition_order[i];
		state.transition_order[i] = swap;
	}
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	/* Match a world with a created playable board 1 before entering EditorLoop. */
	result = zoo_board_close(&state);
	if (result != 0) return 60;
	state.world.board_count = 1;
	state.world.info.current_board = 1;
	state.world.board_len[1] = 0;
	state.world.board_data[1] = NULL;
	state.world.board_external[1] = false;
	zoo_board_create(&state);
	zoo_io_pstring_assign_pascal(state.board.name, sizeof(state.board.name) - 1,
		state.board.io_name_data, &state.board.io_name_length,
		&state.board.io_name_valid, "ROOM", 4);
	/* This is the exact text tile produced by EditorLoop's F4 text-entry path. */
	state.board.tiles[31][12].element = ZOO_E_TEXT_MIN + 5;
	state.board.tiles[31][12].color = (uint8_t) 'A';
	if (zoo_editor_brd_serialize(&state.board, integrated_editor_brd_bytes + 2,
			sizeof(integrated_editor_brd_bytes) - 2, &body_length) != 0) return 61;
	if (body_length > 0xFFFFu || body_length + 2 > sizeof(integrated_editor_brd_bytes)) return 62;
	integrated_editor_brd_bytes[0] = (uint8_t) body_length;
	integrated_editor_brd_bytes[1] = (uint8_t) (body_length >> 8);
	integrated_editor_brd_length_value = (int32_t) body_length + 2;
	result = zoo_editor_brd_transfer(&state, integrated_editor_brd_bytes,
		(size_t) integrated_editor_brd_length_value, &outcome);
	if (result != 0 || outcome != ZOO_EDITOR_BRD_TRANSFER_IMPORTED) return 63;
	if (state.board.name[0] != 'R' || state.board.name[1] != 'O'
			|| state.board.name[2] != 'O' || state.board.name[3] != 'M'
			|| state.board.tiles[31][12].element != ZOO_E_TEXT_MIN + 5
			|| state.board.tiles[31][12].color != (uint8_t) 'A') return 64;
	zoo_board_enter(&state);
	state.game_state = GS_PLAY;
	state.game_paused = true;
	state.game_play_exit_requested = false;
	return integrated_gameplay_run_ticks("888888", 6, NULL, 0, 6,
		false, false, false, true);
}

uint32_t integrated_editor_brd_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_editor_brd_bytes;
}

int32_t integrated_editor_brd_bytes_length(void) {
	return integrated_editor_brd_length_value;
}

uint32_t integrated_f10_bad_world_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_f10_bad_world_bytes;
}

int32_t integrated_f10_bad_world_bytes_capacity(void) {
	return (int32_t) sizeof(integrated_f10_bad_world_bytes);
}

int32_t run_integrated_f10_checks(int32_t malformed_world_length) {
	zoo_io_handle bad_world_handle;
	uint8_t *entry;
	int16_t i, j, random_index, score_position;
	uint16_t swap;
	int result;
	static const uint8_t name[] = "KERRY";

	if (malformed_world_length != (int32_t) sizeof(integrated_f10_bad_world_bytes)) return 70;
	integrated_gameplay_capture_reset();
	integrated_f10_bad_world_load_result_value = -99;
	memset(&integrated_f10_high_score_table, 0, sizeof(integrated_f10_high_score_table));
	memset(integrated_f10_high_score_save_bytes, 0, sizeof(integrated_f10_high_score_save_bytes));
	/* Match the Pascal startup order before the failed startup-world load. */
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		state.transition_order[i] = (uint16_t) i;
	}
	for (i = 0; i < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; i++) {
		random_index = integrated_gameplay_random(&state, ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT);
		swap = state.transition_order[random_index];
		state.transition_order[random_index] = state.transition_order[i];
		state.transition_order[i] = swap;
	}
	state.game_state = GS_PLAY;
	zoo_world_create(&state);
	bad_world_handle = zoo_io_open_file_mem(integrated_f10_bad_world_bytes,
		sizeof(integrated_f10_bad_world_bytes), MODE_READ);
	integrated_f10_bad_world_load_result_value = zoo_world_load(&state, &bad_world_handle, false);
	if (integrated_f10_bad_world_load_result_value != ZOO_ERROR_WRONGVER) return 71;

	/* GameTitleLoop's failed startup WorldLoad falls back to WorldCreate. */
	zoo_world_create(&state);
	zoo_io_pstring_assign_pascal(state.world.info.name, sizeof(state.world.info.name) - 1,
		state.world.info.io_name_data, &state.world.info.io_name_length,
		&state.world.info.io_name_valid, "F10ROOM", 7);
	state.world.info.health = 0;
	state.world.info.score = 1234;
	zoo_board_enter(&state);
	state.game_state = GS_PLAY;
	state.game_paused = false;
	state.tick_speed = 0;
	integrated_gameplay_initial_paused = false;
	/* Seed observable Pascal shortstring tails, then exercise missing-.HI reset. */
	zoo_high_score_table_init(&integrated_f10_high_score_table);
	for (i = 0; i < ZOO_HIGH_SCORE_COUNT; i++) {
		entry = integrated_f10_high_score_table.bytes
			+ (size_t) i * ZOO_HIGH_SCORE_ENTRY_BYTES;
		entry[0] = ZOO_HIGH_SCORE_NAME_BYTES;
		for (j = 0; j < ZOO_HIGH_SCORE_NAME_BYTES; j++) {
			entry[1 + j] = (uint8_t) ('A' + (((i * 7) + j) % 26));
		}
	}
	if (zoo_high_score_table_load(&integrated_f10_high_score_table, NULL, 0, false)
			!= ZOO_ERROR_INVAL) return 72;
	integrated_gameplay_post_loop_key_pressed = 13;
	result = integrated_gameplay_run_ticks(NULL, 0, NULL, 0, 2,
		false, false, false, true);
	if (result != 0) return result;
	score_position = zoo_high_score_insertion_position(&integrated_f10_high_score_table,
		state.world.info.score);
	if (score_position != 0) return 73;
	result = zoo_high_score_insert_at(&integrated_f10_high_score_table, score_position,
		state.world.info.score, name, sizeof(name) - 1);
	if (result != 0) return 74;
	result = zoo_high_score_table_save(&integrated_f10_high_score_table,
		integrated_f10_high_score_save_bytes, sizeof(integrated_f10_high_score_save_bytes));
	if (result != 0) return 75;
	return 0;
}

int32_t integrated_f10_bad_world_load_result(void) {
	return integrated_f10_bad_world_load_result_value;
}

uint32_t integrated_f10_high_score_save_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_f10_high_score_save_bytes;
}

int32_t integrated_f10_high_score_save_length(void) {
	return (int32_t) sizeof(integrated_f10_high_score_save_bytes);
}

int32_t integrated_gameplay_mutate_for_f08_restore(void) {
	state.world.info.health = 77;
	state.world.info.ammo = 42;
	zoo_board_change(&state, 0);
	if (state.error_value != 0) return 46;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'T', 3)) return 47;
	return 0;
}

int32_t integrated_gameplay_reload_f08_save_from_pascal(int32_t length) {
	zoo_io_handle load_handle;
	if (length <= 0 || length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 48;
	load_handle = zoo_io_open_file_mem(integrated_gameplay_reload_input_bytes,
		(size_t) length, MODE_READ);
	integrated_gameplay_reload_result_value = zoo_world_load(&state, &load_handle, false);
	if (integrated_gameplay_reload_result_value != 0) return 49;
	zoo_board_change(&state, state.return_board_id);
	if (state.error_value != 0) return 50;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'T', 4)) return 51;
	return 0;
}

int32_t integrated_gameplay_reload_f08_world_from_pascal(int32_t length) {
	zoo_io_handle load_handle;
	if (length <= 0 || length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 52;
	load_handle = zoo_io_open_file_mem(integrated_demo_world_bytes,
		(size_t) length, MODE_READ);
	integrated_gameplay_reload_result_value = zoo_world_load(&state, &load_handle, false);
	if (integrated_gameplay_reload_result_value != 0) return 53;
	zoo_board_change(&state, state.return_board_id);
	if (state.error_value != 0) return 54;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'T', 5)) return 55;
	return 0;
}

uint32_t integrated_gameplay_reload_input_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_reload_input_bytes;
}

int32_t integrated_gameplay_reload_input_capacity(void) {
	return INTEGRATED_GAMEPLAY_SAVE_CAPACITY;
}

int32_t integrated_gameplay_reload_from_pascal_save(int32_t length) {
	zoo_io_handle load_handle;
	if (length <= 0 || length > INTEGRATED_GAMEPLAY_SAVE_CAPACITY) return 14;
	load_handle = zoo_io_open_file_mem(integrated_gameplay_reload_input_bytes,
		(size_t) length, MODE_READ);
	integrated_gameplay_reload_result_value = zoo_world_load(&state, &load_handle, false);
	if (integrated_gameplay_reload_result_value != 0) return 15;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'T', 3)) return 16;
	return 0;
}

int32_t integrated_gameplay_finish_roundtrip(void) {
	zoo_io_handle save_handle;
	save_handle = zoo_io_open_file_mem(integrated_gameplay_roundtrip_save_bytes,
		INTEGRATED_GAMEPLAY_SAVE_CAPACITY, MODE_WRITE);
	integrated_gameplay_roundtrip_save_result_value = zoo_world_save(&state, &save_handle);
	integrated_gameplay_roundtrip_save_length_value = (int32_t) save_handle.func_tell(&save_handle);
	if (integrated_gameplay_roundtrip_save_length_value >= INTEGRATED_GAMEPLAY_SAVE_CAPACITY) {
		integrated_gameplay_roundtrip_save_overflow_value = 1;
		return 17;
	}
	if (integrated_gameplay_roundtrip_save_result_value != 0) return 18;
	if (!capture_integrated_gameplay_snapshot(
			integrated_gameplay_snapshot_count_value, 'F', 0)) return 19;
	return 0;
}

uint32_t integrated_gameplay_roundtrip_save_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_roundtrip_save_bytes;
}

int32_t integrated_gameplay_roundtrip_save_length(void) {
	return integrated_gameplay_roundtrip_save_length_value;
}

int32_t integrated_gameplay_roundtrip_save_result(void) {
	return integrated_gameplay_roundtrip_save_result_value;
}

int32_t integrated_gameplay_roundtrip_save_overflow(void) {
	return integrated_gameplay_roundtrip_save_overflow_value;
}

int32_t integrated_gameplay_reload_result(void) {
	return integrated_gameplay_reload_result_value;
}

uint32_t integrated_gameplay_output_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_snapshots[integrated_gameplay_snapshot_count_value - 1];
}

uint32_t integrated_gameplay_snapshots_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_snapshots[0];
}

uint32_t integrated_gameplay_checkpoint_headers_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_checkpoint_headers;
}

uint32_t integrated_gameplay_snapshot_lengths_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_snapshot_lengths;
}

int32_t integrated_gameplay_snapshot_stride(void) {
	return INTEGRATED_GAMEPLAY_MAX_FIELDS;
}

int32_t integrated_gameplay_output_length(void) {
	return integrated_gameplay_snapshot_lengths[integrated_gameplay_snapshot_count_value - 1];
}

int32_t integrated_gameplay_snapshot_count(void) {
	return integrated_gameplay_snapshot_count_value;
}

uint32_t integrated_gameplay_sound_events_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_sound_events;
}

int32_t integrated_gameplay_sound_event_count(void) {
	return integrated_gameplay_sound_event_count_value;
}

int32_t integrated_gameplay_sound_event_overflow(void) {
	return integrated_gameplay_sound_event_overflow_value;
}

int32_t integrated_gameplay_sound_event_stride(void) {
	return INTEGRATED_GAMEPLAY_SOUND_EVENT_STRIDE;
}

uint32_t integrated_gameplay_cell_events_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_cell_events;
}

uint32_t integrated_gameplay_cell_context_events_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_cell_context_events;
}

int32_t integrated_gameplay_cell_context_stride(void) {
	return 8;
}

int32_t integrated_gameplay_cell_idempotent_write_count(void) {
	return integrated_gameplay_cell_idempotent_write_count_value;
}

int32_t integrated_gameplay_cell_event_count(void) {
	return integrated_gameplay_cell_event_count_value;
}

int32_t integrated_gameplay_cell_event_overflow(void) {
	return integrated_gameplay_cell_event_overflow_value;
}

int32_t integrated_gameplay_cell_event_stride(void) {
	return INTEGRATED_GAMEPLAY_CELL_EVENT_STRIDE;
}

uint32_t integrated_gameplay_save_bytes_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_save_bytes;
}

int32_t integrated_gameplay_save_length(void) {
	return integrated_gameplay_save_length_value;
}

int32_t integrated_gameplay_save_result(void) {
	return integrated_gameplay_save_result_value;
}

int32_t integrated_gameplay_save_overflow(void) {
	return integrated_gameplay_save_overflow_value;
}

uint32_t integrated_gameplay_trace_ptr(void) {
	return (uint32_t) (uintptr_t) integrated_gameplay_trace;
}
