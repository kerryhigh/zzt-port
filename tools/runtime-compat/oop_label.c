#include <stddef.h>
#include <stdint.h>
#include <zoo.h>

int strncmp(const char *left, const char *right, size_t count);

#include "zoo_stat_move_production.inc"
#include "../../third_party/libzoo/src/libzoo/zoo_game_io.c"
#include "../../third_party/libzoo/src/libzoo/zoo_oop.c"
#include "../../third_party/libzoo/src/libzoo/zoo_sound.c"

const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
	[ZOO_E_EMPTY] = {.color = 0x70, .walkable = true, .cycle = -1, .oop_strip_name = "EMPTY"},
	[3] = {.oop_strip_name = "MONITOR"},
	[4] = {.oop_strip_name = "PLAYER"},
	[5] = {.oop_strip_name = "AMMO"},
	[6] = {.oop_strip_name = "TORCH"},
	[7] = {.oop_strip_name = "GEM"},
	[ZOO_E_KEY] = {.color = ZOO_COLOR_CHOICE_ON_BLACK, .cycle = -1, .oop_strip_name = "KEY"},
	[ZOO_E_SOLID] = {.color = ZOO_COLOR_CHOICE_ON_BLACK, .cycle = -1, .oop_strip_name = "SOLID"},
	[ZOO_E_DOOR] = {.color = ZOO_COLOR_WHITE_ON_CHOICE, .cycle = -1, .oop_strip_name = "DOOR"},
	[10] = {.oop_strip_name = "SCROLL"},
	[11] = {.oop_strip_name = "PASSAGE"},
	[12] = {.oop_strip_name = "DUPLICATOR"},
	[13] = {.oop_strip_name = "BOMB"},
	[14] = {.oop_strip_name = "ENERGIZER"},
	[15] = {.oop_strip_name = "STAR"},
	[16] = {.oop_strip_name = "CLOCKWISE"},
	[17] = {.oop_strip_name = "COUNTER"},
	[18] = {.oop_strip_name = "BULLET"},
	[ZOO_E_WATER] = {.color = 0xF9, .cycle = -1, .oop_strip_name = "WATER"},
	[20] = {.oop_strip_name = "FOREST"},
	[22] = {.oop_strip_name = "NORMAL"},
	[23] = {.oop_strip_name = "BREAKABLE"},
	[24] = {.oop_strip_name = "BOULDER"},
	[25] = {.oop_strip_name = "SLIDERNS"},
	[26] = {.oop_strip_name = "SLIDEREW"},
	[27] = {.oop_strip_name = "FAKE"},
	[28] = {.oop_strip_name = "INVISIBLE"},
	[29] = {.oop_strip_name = "BLINKWALL"},
	[30] = {.oop_strip_name = "TRANSPORTER"},
	[31] = {.oop_strip_name = "LINE"},
	[32] = {.oop_strip_name = "RICOCHET"},
	[34] = {.oop_strip_name = "BEAR"},
	[35] = {.oop_strip_name = "RUFFIAN"},
	[36] = {.oop_strip_name = "OBJECT"},
	[37] = {.oop_strip_name = "SLIME"},
	[38] = {.oop_strip_name = "SHARK"},
	[ZOO_E_SPINNING_GUN] = {.oop_strip_name = "SPINNINGGUN"},
	[40] = {.oop_strip_name = "PUSHER"},
	[41] = {.oop_strip_name = "LION"},
	[42] = {.oop_strip_name = "TIGER"},
	[44] = {.oop_strip_name = "HEAD"},
	[45] = {.oop_strip_name = "SEGMENT"}
};

static zoo_state test_state;
static uint8_t oop_if_outputs[55];
static uint8_t oop_any_outputs[(ZOO_MAX_ELEMENT + 1) * 2];
static uint8_t oop_bind_selector_outputs[5 * 5];
static int16_t oop_counter_outputs[24 * 3];
static int16_t oop_char_outputs[8 * 4];
static int16_t oop_put_outputs[8 * 10];
static uint8_t oop_change_outputs[(ZOO_BOARD_WIDTH + 1) * (ZOO_BOARD_HEIGHT + 1) * 2];
static uint8_t oop_flag_capture_outputs[2 * ZOO_MAX_FLAG * (ZOO_LEN_FLAG + 1) + 7 * sizeof(int16_t)];
static uint8_t oop_send_capture_outputs[23];
static int16_t oop_flow_outputs[11 * 11];
static int oop_counter_output_count;
static char oop_any_name[64];
static uint8_t oop_value_input[64];
static uint8_t oop_line_input[320];
static char oop_line_output[256];
static char oop_title_source[128];
static int16_t oop_value_position_result;
static int16_t oop_line_position_result;
static int16_t direction_random_values[4];
static int direction_random_count;
static int16_t direction_rnd_dx;
static int16_t direction_rnd_dy;
static int16_t direction_seek_dx;
static int16_t direction_seek_dy;
static char *oop_tracked_free_data;
static size_t oop_tracked_free_length;
static int oop_tracked_free_count;
static int shoot_call_count;
static uint8_t shoot_element;
static int16_t shoot_x;
static int16_t shoot_y;
static int16_t shoot_dx;
static int16_t shoot_dy;
static int16_t shoot_param1;
static bool shoot_result;

void free(void *pointer) {
	if (pointer == oop_tracked_free_data && pointer != NULL) {
		size_t i;
		oop_tracked_free_count++;
		for (i = 0; i < oop_tracked_free_length; i++) {
			oop_tracked_free_data[i] = '\0';
		}
	}
}

bool zoo_board_shoot(zoo_state *state, uint8_t element, int16_t x, int16_t y,
	int16_t dx, int16_t dy, int16_t param1) {
	(void) state;
	shoot_call_count++;
	shoot_element = element;
	shoot_x = x;
	shoot_y = y;
	shoot_dx = dx;
	shoot_dy = dy;
	shoot_param1 = param1;
	return shoot_result;
}

static int16_t test_direction_random(zoo_state *state, int16_t max) {
	int16_t value;
	(void) state;
	if (direction_random_count >= 4 || max <= 0) return 0;
	value = direction_random_values[direction_random_count++];
	return (int16_t) ((uint16_t) value % (uint16_t) max);
}

void zoo_calc_direction_rnd(zoo_state *state, int16_t *dx, int16_t *dy) {
	(void) state;
	*dx = direction_rnd_dx;
	*dy = direction_rnd_dy;
}

void zoo_calc_direction_seek(zoo_state *state, int16_t x, int16_t y, int16_t *dx, int16_t *dy) {
	(void) state;
	(void) x;
	(void) y;
	*dx = direction_seek_dx;
	*dy = direction_seek_dy;
}

void zoo_window_append(zoo_text_window *window, const char *text) {
	(void) text;
	if (window->line_count < 0) window->line_count = 0;
	window->line_count++;
}

void zoo_window_append_pascal(zoo_state *state, zoo_text_window *window, const char *text) {
	(void) state;
	zoo_window_append(window, text);
}

static void test_draw_sidebar(zoo_state *state, uint16_t flags) {
	(void) state;
	(void) flags;
}

uint32_t oop_value_input_ptr(void) {
    return (uint32_t) (uintptr_t) oop_value_input;
}

int32_t oop_value_result_position(void) {
	return oop_value_position_result;
}

uint32_t oop_word_result_ptr(void) {
	return (uint32_t) (uintptr_t) test_state.oop_word;
}

int32_t run_oop_word_from_input(int32_t length) {
	int16_t position = 0;
	if (length < 0 || length > (int32_t) sizeof(oop_value_input)) return -32769;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stats[1].data = (char *) oop_value_input;
	test_state.board.stats[1].data_len = (int16_t) length;
	zoo_oop_read_word(&test_state, 1, &position);
	return position;
}

int32_t run_oop_value_from_input(int32_t length, int32_t initial_value) {
    int16_t position = 0;
    if (length < 0 || length > (int32_t) sizeof(oop_value_input)) return -32769;
    memset(&test_state, 0, sizeof(test_state));
    test_state.board.stats[1].data = (char *) oop_value_input;
    test_state.board.stats[1].data_len = (int16_t) length;
    test_state.oop_value = (int16_t) initial_value;
    zoo_oop_read_value(&test_state, 1, &position);
	oop_value_position_result = position;
    return test_state.oop_value;
}

uint32_t oop_line_input_ptr(void) {
	return (uint32_t) (uintptr_t) oop_line_input;
}

uint32_t oop_line_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_line_output;
}

int32_t oop_line_result_position(void) {
	return oop_line_position_result;
}

int32_t run_oop_line_from_input(int32_t length) {
	int16_t position = 0;
	int32_t line_length = 0;
	if (length < 0 || length > (int32_t) sizeof(oop_line_input)) return -32769;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stats[1].data = (char *) oop_line_input;
	test_state.board.stats[1].data_len = (int16_t) length;
	memset(oop_line_output, 0xCC, sizeof(oop_line_output));
	zoo_oop_read_line_to_end(&test_state, 1, &position, oop_line_output, 255);
	while (line_length < 255 && oop_line_output[line_length] != '\0') line_length++;
	oop_line_position_result = position;
	return line_length;
}

static int run_oop_window_title_case(const char *source, const char *default_name,
	const char *expected_title) {
	int16_t position = 0;
	size_t length = strlen(source);
	if (length >= sizeof(oop_title_source)) return 1;
	memset(&test_state, 0, sizeof(test_state));
	memset(oop_title_source, 0, sizeof(oop_title_source));
	memcpy(oop_title_source, source, length);
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = oop_title_source;
	test_state.board.stats[1].data_len = (int16_t) length;
	zoo_oop_execute(&test_state, 1, &position, default_name);
	if (!test_state.object_window_request) return 2;
	return strcmp(test_state.object_window.title, expected_title) == 0 ? 0 : 3;
}

int32_t run_oop_window_title_checks(void) {
	// Prime the C stack's reusable name buffer with a custom title before the default-title case.
	if (run_oop_window_title_case("@Custom title\rFirst line\rSecond line\r",
		"Interaction", "Custom title") != 0) return 1;
	if (run_oop_window_title_case("First line\rSecond line\r",
		"Scroll", "Scroll") != 0) return 2;
	if (run_oop_window_title_case("@\rFirst line\rSecond line\r",
		"Scroll", "Interaction") != 0) return 3;
	return 0;
}

void *memcpy(void *destination, const void *source, size_t count) {
    uint8_t *dst = (uint8_t *) destination;
    const uint8_t *src = (const uint8_t *) source;
    size_t i;
    for (i = 0; i < count; i++) dst[i] = src[i];
    return destination;
}

void *memset(void *destination, int value, size_t count) {
    uint8_t *dst = (uint8_t *) destination;
    size_t i;
    for (i = 0; i < count; i++) dst[i] = (uint8_t) value;
    return destination;
}

size_t strlen(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') length++;
    return length;
}

size_t strnlen(const char *text, size_t limit) {
    size_t length = 0;
    while (length < limit && text[length] != '\0') length++;
    return length;
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

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) { left++; right++; }
    return (uint8_t) *left - (uint8_t) *right;
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

char *strchr(const char *text, int value) {
    char match = (char) value;
    for (;;) {
        if (*text == match) return (char *) text;
        if (*text == '\0') return NULL;
        text++;
    }
}

char *strncpy(char *destination, const char *source, size_t count) {
    size_t i;
    for (i = 0; i < count && source[i] != '\0'; i++) destination[i] = source[i];
    for (; i < count; i++) destination[i] = '\0';
    return destination;
}

char *strncat(char *destination, const char *source, size_t count) {
    size_t length = strlen(destination);
    size_t i;
    for (i = 0; i < count && source[i] != '\0'; i++) destination[length + i] = source[i];
    destination[length + i] = '\0';
    return destination;
}

static void setup_target(const char *target_data, size_t target_length) {
    memset(&test_state, 0, sizeof(test_state));
    test_state.board.stat_count = 2;
    test_state.board.stats[1].data = (char *) "@SENDER\r";
    test_state.board.stats[1].data_len = 8;
    test_state.board.stats[2].data = (char *) target_data;
    test_state.board.stats[2].data_len = (int16_t) target_length;
}

int run_oop_label_checks(void) {
    static const char target_data[] = "@ABCDEFGHIJKLMNOPQRST\r:UVWXYZABCDEFGHIJKLMN\r";
    static const char wrong_boundary_data[] = "@ABCDEFGHIJKLMNOPQRST\r:UVWXYZABCDEFGHIJKLMNX\r";
    int16_t target_stat;
    int16_t data_pos;

    setup_target(target_data, sizeof(target_data) - 1);
    target_stat = 0;
    data_pos = -1;
    if (!zoo_oop_find_label(&test_state, 1,
            "ABCDEFGHIJKLMNOPQRSTX:UVWXYZABCDEFGHIJKLMN", &target_stat, &data_pos, "\r:")) return 1;
    if (target_stat != 2 || data_pos != 21) return 2;

    setup_target(target_data, sizeof(target_data) - 1);
    target_stat = 0;
    data_pos = -1;
    if (!zoo_oop_find_label(&test_state, 1,
            "ABCDEFGHIJKLMNOPQRST:UVWXYZABCDEFGHIJKLMNX", &target_stat, &data_pos, "\r:")) return 3;
    if (target_stat != 2 || data_pos != 21) return 4;

    setup_target(wrong_boundary_data, sizeof(wrong_boundary_data) - 1);
    target_stat = 0;
    data_pos = -1;
    if (zoo_oop_find_label(&test_state, 1,
            "ABCDEFGHIJKLMNOPQRST:UVWXYZABCDEFGHIJKLMN", &target_stat, &data_pos, "\r:")) return 5;

    return 0;
}

int run_oop_flag_checks(void) {
    memset(&test_state, 0, sizeof(test_state));
    if (zoo_flag_get_id(&test_state, "") != ZOO_MAX_FLAG - 1) return 1;

    memset(&test_state, 0, sizeof(test_state));
    test_state.world.info.flags[0][0] = 'A';
    test_state.world.info.io_flag_valid[0] = true;
    test_state.world.info.io_flag_lengths[0] = 3;
    test_state.world.info.io_flag_data[0][0] = 'A';
    test_state.world.info.io_flag_data[0][1] = 0;
    test_state.world.info.io_flag_data[0][2] = 'B';
    if (zoo_flag_get_id(&test_state, "A") != -1) return 2;

    memset(&test_state, 0, sizeof(test_state));
    test_state.world.info.flags[0][0] = 'D';
    test_state.world.info.flags[0][1] = 'U';
    test_state.world.info.flags[0][2] = 'P';
    test_state.world.info.flags[1][0] = 'D';
    test_state.world.info.flags[1][1] = 'U';
    test_state.world.info.flags[1][2] = 'P';
    if (zoo_flag_get_id(&test_state, "DUP") != 1) return 3;
    zoo_flag_clear(&test_state, "DUP");
    if (test_state.world.info.flags[0][0] != 'D' || test_state.world.info.flags[1][0] != 0) return 4;

    memset(&test_state, 0, sizeof(test_state));
    test_state.world.info.io_flag_valid[0] = true;
    test_state.world.info.io_flag_lengths[0] = 1;
    test_state.world.info.io_flag_data[0][0] = 0;
    test_state.world.info.flags[1][0] = 'B';
    test_state.world.info.flags[1][1] = 'U';
    test_state.world.info.flags[1][2] = 'S';
    test_state.world.info.flags[1][3] = 'Y';
    zoo_flag_set(&test_state, "NEW");
    if (test_state.world.info.flags[0][0] != 0) return 5;
    if (test_state.world.info.flags[2][0] != 'N' || !test_state.world.info.io_flag_valid[2]
            || test_state.world.info.io_flag_lengths[2] != 3
            || memcmp(test_state.world.info.io_flag_data[2], "NEW", 3) != 0) return 6;

    return 0;
}

int run_oop_try_command_checks(void) {
	static const char script[] = "#TRY EAST #CYCLE 5\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[0].x = 11;
	test_state.board.stats[0].y = 10;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = (char *) script;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(script) - 1);
	position = 0;
	test_state.board.tiles[11][10].element = ZOO_E_BOARD_EDGE;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].cycle != 5) return 1;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = (char *) script;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(script) - 1);
	position = 0;
	test_state.board.tiles[11][10].element = ZOO_E_EMPTY;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].cycle != 0) return 2;

	return 0;
}

static int run_oop_if_case(const char *script, int16_t energizer_ticks,
	int16_t stat_x, int16_t stat_y, int16_t player_x, int16_t player_y,
	int16_t obstacle_x, int16_t obstacle_y, uint8_t obstacle_element, uint8_t obstacle_color,
	const char *flag, int16_t expected_cycle, size_t output_index) {
	int16_t position = 0;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[0].x = player_x;
	test_state.board.stats[0].y = player_y;
	test_state.board.stats[1].x = stat_x;
	test_state.board.stats[1].y = stat_y;
	test_state.board.stats[1].data = (char *) script;
	test_state.board.stats[1].data_len = (int16_t) strlen(script);
	test_state.world.info.energizer_ticks = energizer_ticks;
	if (obstacle_x != 0 || obstacle_y != 0) {
		test_state.board.tiles[obstacle_x][obstacle_y].element = obstacle_element;
		test_state.board.tiles[obstacle_x][obstacle_y].color = obstacle_color;
	}
	if (flag != NULL) {
		strncpy(test_state.world.info.flags[0], flag, sizeof(test_state.world.info.flags[0]) - 1);
	}
	zoo_oop_execute(&test_state, 1, &position, "Test");
	oop_if_outputs[output_index] = test_state.board.stats[1].cycle == 7 ? 1 : 0;
	return test_state.board.stats[1].cycle == expected_cycle ? 0 : 1;
}

uint32_t oop_if_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_if_outputs;
}

int32_t run_oop_if_condition_checks(void) {
	if (run_oop_if_case("#IF ENERGIZED #CYCLE 7\r", 1, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 0)) return 1;
	if (run_oop_if_case("#IF ENERGIZED #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 1)) return 2;
	if (run_oop_if_case("#IF NOT ENERGIZED #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 2)) return 3;
	if (run_oop_if_case("#IF NOT ENERGIZED #CYCLE 7\r", 1, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 3)) return 4;
	if (run_oop_if_case("#IF ALLIGNED #CYCLE 7\r", 0, 10, 10, 10, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 4)) return 5;
	if (run_oop_if_case("#IF ALLIGNED #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 5)) return 6;
	if (run_oop_if_case("#IF CONTACT #CYCLE 7\r", 0, 10, 10, 11, 10, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 6)) return 7;
	if (run_oop_if_case("#IF CONTACT #CYCLE 7\r", 0, 10, 10, 12, 10, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 7)) return 8;
	if (run_oop_if_case("#IF BLOCKED NORTH #CYCLE 7\r", 0, 10, 10, 5, 5, 10, 9, ZOO_E_SOLID, 0, NULL, 7, 8)) return 9;
	if (run_oop_if_case("#IF BLOCKED NORTH #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 9)) return 10;
	if (run_oop_if_case("#IF ANY SOLID #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_SOLID, 0, NULL, 7, 10)) return 11;
	if (run_oop_if_case("#IF ANY SOLID #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 11)) return 12;
	if (run_oop_if_case("#IF ANY BLUE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 7, 12)) return 13;
	if (run_oop_if_case("#IF ANY BLUE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x2F, NULL, 0, 13)) return 14;
	if (run_oop_if_case("#IF ANY GREEN DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x2F, NULL, 7, 14)) return 15;
	if (run_oop_if_case("#IF ANY GREEN DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 15)) return 16;
	if (run_oop_if_case("#IF ANY CYAN DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x3F, NULL, 7, 16)) return 17;
	if (run_oop_if_case("#IF ANY CYAN DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 17)) return 18;
	if (run_oop_if_case("#IF ANY RED DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x4F, NULL, 7, 18)) return 19;
	if (run_oop_if_case("#IF ANY RED DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 19)) return 20;
	if (run_oop_if_case("#IF ANY PURPLE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x5F, NULL, 7, 20)) return 21;
	if (run_oop_if_case("#IF ANY PURPLE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 21)) return 22;
	if (run_oop_if_case("#IF ANY YELLOW DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x6F, NULL, 7, 22)) return 23;
	if (run_oop_if_case("#IF ANY YELLOW DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 23)) return 24;
	if (run_oop_if_case("#IF ANY WHITE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x7F, NULL, 7, 24)) return 25;
	if (run_oop_if_case("#IF ANY WHITE DOOR #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_DOOR, 0x1F, NULL, 0, 25)) return 26;
	if (run_oop_if_case("#IF HELLO #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, "HELLO", 7, 26)) return 27;
	if (run_oop_if_case("#IF HELLO #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 27)) return 28;
	if (run_oop_if_case("#IF ALLIGNED #CYCLE 7\r", 0, 10, 10, 5, 10, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 28)) return 29;
	if (run_oop_if_case("#IF CONTACT #CYCLE 7\r", 0, 10, 10, 10, 9, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 29)) return 30;
	if (run_oop_if_case("#IF CONTACT #CYCLE 7\r", 0, 10, 10, 11, 11, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 30)) return 31;
	if (run_oop_if_case("#IF BLOCKED SOUTH #CYCLE 7\r", 0, 10, 10, 5, 5, 10, 11, ZOO_E_SOLID, 0, NULL, 7, 31)) return 32;
	if (run_oop_if_case("#IF BLOCKED WEST #CYCLE 7\r", 0, 10, 10, 5, 5, 9, 10, ZOO_E_EMPTY, 0, NULL, 0, 32)) return 33;
	if (run_oop_if_case("#IF ANY WATER #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_WATER, 0, NULL, 7, 33)) return 34;
	if (run_oop_if_case("#IF ANY WATER #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 34)) return 35;
	if (run_oop_if_case("#IF ANY BLUE KEY #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_KEY, 0x19, NULL, 7, 35)) return 36;
	if (run_oop_if_case("#IF ANY BLUE KEY #CYCLE 7\r", 0, 10, 10, 5, 5, 3, 3, ZOO_E_KEY, 0x1C, NULL, 0, 36)) return 37;
	if (run_oop_if_case("#IF BLOCKED EAST #CYCLE 7\r", 0, 10, 10, 5, 5, 11, 10, ZOO_E_SOLID, 0, NULL, 7, 37)) return 38;
	if (run_oop_if_case("#IF BLOCKED EAST #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 38)) return 39;
	if (run_oop_if_case("#IF BLOCKED WEST #CYCLE 7\r", 0, 10, 10, 5, 5, 9, 10, ZOO_E_SOLID, 0, NULL, 7, 39)) return 40;
	if (run_oop_if_case("#IF BLOCKED SOUTH #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 40)) return 41;
	if (run_oop_if_case("#IF BLOCKED WEST #CYCLE 7\r", 0, 1, 10, 5, 5, 0, 10, ZOO_E_BOARD_EDGE, 0, NULL, 7, 41)) return 42;
	if (run_oop_if_case("#IF BLOCKED EAST #CYCLE 7\r", 0, 60, 10, 5, 5, 61, 10, ZOO_E_BOARD_EDGE, 0, NULL, 7, 42)) return 43;
	if (run_oop_if_case("#IF BLOCKED NORTH #CYCLE 7\r", 0, 10, 1, 5, 5, 10, 0, ZOO_E_BOARD_EDGE, 0, NULL, 7, 43)) return 44;
	if (run_oop_if_case("#IF BLOCKED SOUTH #CYCLE 7\r", 0, 10, 25, 5, 5, 10, 26, ZOO_E_BOARD_EDGE, 0, NULL, 7, 44)) return 45;
	if (run_oop_if_case("#IF ANY SOLID #CYCLE 7\r", 0, 10, 10, 5, 5, 60, 25, ZOO_E_SOLID, 0, NULL, 7, 45)) return 46;
	if (run_oop_if_case("#IF ANY SOLID #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 10, ZOO_E_SOLID, 0, NULL, 0, 46)) return 47;
	if (run_oop_if_case("#IF NOT NOT ENERGIZED #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 47)) return 48;
	if (run_oop_if_case("#IF NOT NOT ENERGIZED #CYCLE 7\r", 1, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 48)) return 49;
	if (run_oop_if_case("#IF NOT NOT NOT ENERGIZED #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 7, 49)) return 50;
	if (run_oop_if_case("#IF NOT NOT NOT ENERGIZED #CYCLE 7\r", 1, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 50)) return 51;
	if (run_oop_if_case("#IF NOT NOT BLOCKED NORTH #CYCLE 7\r", 0, 10, 10, 5, 5, 10, 9, ZOO_E_SOLID, 0, NULL, 7, 51)) return 52;
	if (run_oop_if_case("#IF NOT NOT BLOCKED NORTH #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 52)) return 53;
	if (run_oop_if_case("#IF NOT NOT HELLO #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, "HELLO", 7, 53)) return 54;
	if (run_oop_if_case("#IF NOT NOT HELLO #CYCLE 7\r", 0, 10, 10, 5, 5, 0, 0, ZOO_E_EMPTY, 0, NULL, 0, 54)) return 55;
	return 0;
}

uint32_t oop_any_name_ptr(void) {
	return (uint32_t) (uintptr_t) oop_any_name;
}

uint32_t oop_any_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_any_outputs;
}

static uint8_t run_oop_any_presence(const char *script, int16_t element_id,
	int16_t filler_element, bool place_match) {
	int16_t position = 0;
	int16_t x, y;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[0].x = 5;
	test_state.board.stats[0].y = 5;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = (char *) script;
	test_state.board.stats[1].data_len = (int16_t) strlen(script);
	for (x = 1; x <= ZOO_BOARD_WIDTH; x++) {
		for (y = 1; y <= ZOO_BOARD_HEIGHT; y++) {
			test_state.board.tiles[x][y].element = (uint8_t) filler_element;
		}
	}
	if (place_match) {
		test_state.board.tiles[3][3].element = (uint8_t) element_id;
	}
	zoo_oop_execute(&test_state, 1, &position, "ANY element name");
	return test_state.board.stats[1].cycle == 7 ? 1 : 0;
}

int32_t run_oop_any_element_check(int32_t element_id) {
	static const char prefix[] = "#IF ANY ";
	static const char suffix[] = " #CYCLE 7\r";
	char script[128];
	size_t name_length = strlen(oop_any_name);
	size_t script_length;
	int16_t filler_element;
	if (element_id < 0 || element_id > ZOO_MAX_ELEMENT || name_length == 0
		|| name_length >= sizeof(oop_any_name)) return 1;
	script_length = (sizeof(prefix) - 1) + name_length + (sizeof(suffix) - 1);
	memcpy(script, prefix, sizeof(prefix) - 1);
	memcpy(script + sizeof(prefix) - 1, oop_any_name, name_length);
	memcpy(script + sizeof(prefix) - 1 + name_length, suffix, sizeof(suffix) - 1);
	script[script_length] = '\0';
	filler_element = element_id == ZOO_E_EMPTY ? ZOO_E_SOLID : ZOO_E_EMPTY;
	oop_any_outputs[element_id * 2] = run_oop_any_presence(script, (int16_t) element_id,
		filler_element, true);
	oop_any_outputs[element_id * 2 + 1] = run_oop_any_presence(script, (int16_t) element_id,
		filler_element, false);
	return 0;
}

static int16_t *test_counter_pointer(int counter_id) {
	switch (counter_id) {
	case 0: return &test_state.world.info.health;
	case 1: return &test_state.world.info.ammo;
	case 2: return &test_state.world.info.gems;
	case 3: return &test_state.world.info.torches;
	case 4: return &test_state.world.info.score;
	default: return &test_state.world.info.board_time_sec;
	}
}

static int run_oop_counter_case(const char *script, int counter_id,
	int16_t initial_value, int16_t expected_value, int16_t expected_cycle) {
	int16_t position = 0;
	int16_t *counter;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = (char *) script;
	test_state.board.stats[1].data_len = (int16_t) strlen(script);
	counter = test_counter_pointer(counter_id);
	*counter = initial_value;
	test_state.func_draw_sidebar = test_draw_sidebar;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (oop_counter_output_count < 24) {
		int output_index = oop_counter_output_count * 3;
		oop_counter_outputs[output_index] = (int16_t) (oop_counter_output_count + 1);
		oop_counter_outputs[output_index + 1] = *counter;
		oop_counter_outputs[output_index + 2] = test_state.board.stats[1].cycle;
		oop_counter_output_count++;
	}
	return (*counter == expected_value
		&& test_state.board.stats[1].cycle == expected_cycle) ? 0 : 1;
}

uint32_t oop_counter_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_counter_outputs;
}

int32_t run_oop_counter_overflow_checks(void) {
	static const char *give_scripts[] = {
		"#GIVE HEALTH 1 #CYCLE 5\r",
		"#GIVE AMMO 1 #CYCLE 5\r",
		"#GIVE GEMS 1 #CYCLE 5\r",
		"#GIVE TORCHES 1 #CYCLE 5\r",
		"#GIVE SCORE 1 #CYCLE 5\r",
		"#GIVE TIME 1 #CYCLE 5\r"
	};
	static const char *take_scripts[] = {
		"#TAKE HEALTH 1 #CYCLE 5\r",
		"#TAKE AMMO 1 #CYCLE 5\r",
		"#TAKE GEMS 1 #CYCLE 5\r",
		"#TAKE TORCHES 1 #CYCLE 5\r",
		"#TAKE SCORE 1 #CYCLE 5\r",
		"#TAKE TIME 1 #CYCLE 5\r"
	};
	int counter_id;
	oop_counter_output_count = 0;
	for (counter_id = 0; counter_id < 6; counter_id++) {
		if (run_oop_counter_case(give_scripts[counter_id], counter_id, 32767, 32767, 5))
			return 1 + counter_id * 4;
		if (run_oop_counter_case(give_scripts[counter_id], counter_id, 32766, 32767, 0))
			return 2 + counter_id * 4;
		if (run_oop_counter_case(take_scripts[counter_id], counter_id, 0, 0, 5))
			return 3 + counter_id * 4;
		if (run_oop_counter_case(take_scripts[counter_id], counter_id, 1, 0, 0))
			return 4 + counter_id * 4;
	}
	return 0;
}

int32_t run_oop_zap_restore_checks(void) {
	static char source[] = "#ZAP FOO\r:FOO\r'FOO\r#RESTORE FOO\r#END\r";
	int16_t position = 0;
	size_t i;
	int restored_count = 0;
	int apostrophe_count = 0;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(source) - 1);
	zoo_oop_execute(&test_state, 1, &position, "Test");

	for (i = 0; i + 5 < sizeof(source) - 1; i++) {
		if (source[i] == '\r' && source[i + 1] == ':'
			&& source[i + 2] == 'F' && source[i + 3] == 'O' && source[i + 4] == 'O'
			&& source[i + 5] == '\r') {
			restored_count++;
		}
		if (source[i] == '\'') apostrophe_count++;
	}
	if (restored_count != 2) return 1;
	if (apostrophe_count != 0) return 2;
	return 0;
}

int32_t run_oop_bind_self_checks(void) {
	static char source[] = "#BIND SELF\r#CYCLE 7\r#END\r";
	static char other_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char target_source[] = "@TARGET\r#CYCLE 9\r#END\r";
	static char alias_source[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	static char alias_target_source[] = "@TARGET\r#CYCLE 9\r#END\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(source) - 1);
	position = 0;
	oop_tracked_free_data = source;
	oop_tracked_free_length = sizeof(source) - 1;
	oop_tracked_free_count = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	oop_tracked_free_data = NULL;
	if (test_state.board.stats[1].data != source) return 1;
	if (test_state.board.stats[1].data_len != (int16_t) (sizeof(source) - 1)) return 2;
	if (test_state.board.stats[1].cycle != 7) return 3;
	if (oop_tracked_free_count != 0) return 4;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 2;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = other_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(other_source) - 1);
	test_state.board.stats[2].x = 11;
	test_state.board.stats[2].y = 10;
	test_state.board.stats[2].data = target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(target_source) - 1);
	position = 0;
	oop_tracked_free_data = other_source;
	oop_tracked_free_length = sizeof(other_source) - 1;
	oop_tracked_free_count = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	oop_tracked_free_data = NULL;
	if (test_state.board.stats[1].data != target_source) return 5;
	if (test_state.board.stats[1].data_len != (int16_t) (sizeof(target_source) - 1)) return 6;
	if (test_state.board.stats[1].cycle != 9) return 7;
	if (oop_tracked_free_count != 1 || other_source[0] != '\0') return 8;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 3;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = alias_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(alias_source) - 1);
	test_state.board.stats[2].x = 11;
	test_state.board.stats[2].y = 10;
	test_state.board.stats[2].data = alias_target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(alias_target_source) - 1);
	test_state.board.stats[3].x = 12;
	test_state.board.stats[3].y = 10;
	test_state.board.stats[3].data = alias_source;
	test_state.board.stats[3].data_len = (int16_t) (sizeof(alias_source) - 1);
	position = 0;
	oop_tracked_free_data = alias_source;
	oop_tracked_free_length = sizeof(alias_source) - 1;
	oop_tracked_free_count = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	oop_tracked_free_data = NULL;
	if (test_state.board.stats[1].data != alias_target_source) return 9;
	if (test_state.board.stats[1].cycle != 9) return 10;
	if (test_state.board.stats[1].data_len != (int16_t) (sizeof(alias_target_source) - 1)) return 11;
	if (test_state.board.stats[1].data_pos != 0) return 12;
	if (test_state.board.stats[3].data != alias_source) return 13;
	if (test_state.board.stats[3].data_len != (int16_t) (sizeof(alias_source) - 1)) return 14;
	if (oop_tracked_free_count != 0 || alias_source[0] != '#') return 15;
	position = 13;
	zoo_oop_execute(&test_state, 3, &position, "Bind alias follower");
	if (test_state.board.stats[3].cycle != 5) return 16;
	if (test_state.board.stats[3].data != alias_source) return 17;
	if (test_state.board.stats[3].data_len != (int16_t) (sizeof(alias_source) - 1)) return 18;
	position = 13;
	zoo_oop_execute(&test_state, 3, &position, "Bind alias follower");
	if (test_state.board.stats[3].cycle != 5) return 19;
	return 0;
}

static int32_t run_oop_bind_selector_case(int32_t case_id) {
	char bind_others[] = "#BIND OTHERS\r#CYCLE 5\r#END\r";
	char others_target[] = "@OTHER\r#CYCLE 9\r#END\r";
	char others_from_second[] = "#BIND OTHERS\r#CYCLE 5\r#END\r";
	char others_second_target[] = "@FIRST\r#CYCLE 13\r#END\r";
	char bind_all[] = "#BIND ALL\r#CYCLE 5\r#END\r";
	char all_target[] = "@FIRST\r#CYCLE 7\r#END\r";
	char bind_named[] = "#BIND TARGET\r#CYCLE 5\r#END\r";
	char named_target_first[] = "@target\r#CYCLE 9\r#END\r";
	char named_target_second[] = "@TARGET\r#CYCLE 11\r#END\r";
	char bind_missing[] = "#BIND MISSING\r#CYCLE 5\r#END\r";
	char *source;
	char *expected_target;
	char *second_target = NULL;
	char *third_target = NULL;
	int16_t source_stat;
	int16_t stat_count;
	int16_t position = 0;
	int16_t expected_cycles[3] = {0, 0, 0};
	int16_t expected_length;
	int expected_free_count;
	int i;
	size_t source_length;
	uint8_t *output = &oop_bind_selector_outputs[(case_id - 1) * 5];

	memset(&test_state, 0, sizeof(test_state));
	if (case_id == 1) {
		source = bind_others;
		expected_target = others_target;
		source_stat = 1;
		stat_count = 2;
		expected_cycles[0] = 9;
		expected_length = (int16_t) (sizeof(others_target) - 1);
		expected_free_count = 1;
		second_target = others_target;
	} else if (case_id == 2) {
		source = bind_all;
		expected_target = all_target;
		source_stat = 2;
		stat_count = 2;
		expected_cycles[1] = 7;
		expected_length = (int16_t) (sizeof(all_target) - 1);
		expected_free_count = 1;
		second_target = all_target;
	} else if (case_id == 3) {
		source = bind_named;
		expected_target = named_target_first;
		source_stat = 1;
		stat_count = 3;
		expected_cycles[0] = 9;
		expected_length = (int16_t) (sizeof(named_target_first) - 1);
		expected_free_count = 1;
		second_target = named_target_first;
		third_target = named_target_second;
	} else if (case_id == 4) {
		source = bind_missing;
		expected_target = bind_missing;
		source_stat = 1;
		stat_count = 1;
		expected_cycles[0] = 5;
		expected_length = (int16_t) (sizeof(bind_missing) - 1);
		expected_free_count = 0;
	} else if (case_id == 5) {
		source = others_from_second;
		expected_target = others_second_target;
		source_stat = 2;
		stat_count = 2;
		expected_cycles[1] = 13;
		expected_length = (int16_t) (sizeof(others_second_target) - 1);
		expected_free_count = 1;
		second_target = others_second_target;
	} else {
		return 1;
	}

	test_state.board.stat_count = stat_count;
	for (i = 1; i <= stat_count; i++) {
		test_state.board.stats[i].x = (int16_t) (10 + i);
		test_state.board.stats[i].y = 10;
	}
	source_length = strlen(source);
	test_state.board.stats[source_stat].data = source;
	test_state.board.stats[source_stat].data_len = (int16_t) source_length;
	if (second_target != NULL) {
		int16_t target_stat = (case_id == 2 || case_id == 5) ? 1 : 2;
		test_state.board.stats[target_stat].data = second_target;
		test_state.board.stats[target_stat].data_len = (int16_t) strlen(second_target);
	}
	if (third_target != NULL) {
		test_state.board.stats[3].data = third_target;
		test_state.board.stats[3].data_len = (int16_t) strlen(third_target);
	}
	oop_tracked_free_data = source;
	oop_tracked_free_length = source_length;
	oop_tracked_free_count = 0;
	zoo_oop_execute(&test_state, source_stat, &position, "Bind selector");
	oop_tracked_free_data = NULL;

	output[0] = (uint8_t) case_id;
	output[1] = (uint8_t) test_state.board.stats[1].cycle;
	output[2] = (uint8_t) test_state.board.stats[2].cycle;
	output[3] = (uint8_t) test_state.board.stats[3].cycle;
	output[4] = (uint8_t) test_state.board.stats[source_stat].data_len;
	if (test_state.board.stats[source_stat].data != expected_target) return 2;
	if (test_state.board.stats[source_stat].data_len != expected_length) return 3;
	if (test_state.board.stats[source_stat].cycle != expected_cycles[source_stat - 1]) return 4;
	if (oop_tracked_free_count != expected_free_count) return 5;
	for (i = 1; i <= 3; i++) {
		if (test_state.board.stats[i].cycle != expected_cycles[i - 1]) return 6 + i;
	}
	return 0;
}

int32_t run_oop_bind_selector_checks(void) {
	int32_t case_id;
	int32_t result;
	for (case_id = 1; case_id <= 5; case_id++) {
		result = run_oop_bind_selector_case(case_id);
		if (result != 0) return case_id * 100 + result;
	}
	return 0;
}

uint32_t oop_bind_selector_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_bind_selector_outputs;
}

static void reset_oop_flow_case(char *source, int16_t source_length, uint8_t east_element) {
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].cycle = 3;
	test_state.board.stats[1].data = source;
	test_state.board.stats[1].data_len = source_length;
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	test_state.board.tiles[10][10].color = 0x0F;
	test_state.board.tiles[11][10].element = east_element;
	test_state.board.tiles[11][10].color = 0x2A;
	test_state.world.info.health = 10;
}

static int16_t capture_oop_flow_output(int16_t case_id, int16_t position, int16_t output_index) {
	int16_t *out = &oop_flow_outputs[output_index * 11];
	out[0] = case_id;
	out[1] = position;
	out[2] = test_state.board.stats[1].cycle;
	out[3] = test_state.world.info.health;
	out[4] = test_state.board.stats[1].p2;
	out[5] = test_state.board.stats[1].x;
	out[6] = test_state.board.stats[1].y;
	out[7] = test_state.board.stats[1].step_x;
	out[8] = test_state.board.stats[1].step_y;
	out[9] = test_state.board.tiles[10][10].element;
	out[10] = test_state.board.tiles[11][10].element;
	return (int16_t) (output_index + 1);
}

uint32_t oop_flow_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_flow_outputs;
}

int32_t run_oop_flow_capture_checks(void) {
	static char idle_source[] = "#IDLE\r#CYCLE 7\r";
	static char end_source[] = "#END\r#CYCLE 9\r";
	static char endgame_source[] = "#ENDGAME\r#CYCLE 7\r";
	static char lock_source[] = "#LOCK\r#IDLE\r#UNLOCK\r#END\r";
	static char restart_source[] = "#RESTART\r#CYCLE 9\r";
	static char go_source[] = "#GO EAST #CYCLE 5\r";
	static char try_source[] = "#TRY EAST #CYCLE 5\r";
	static char *const sources[7] = {
		idle_source, end_source, endgame_source, lock_source,
		restart_source, go_source, try_source
	};
	static const int16_t source_lengths[7] = {
		(int16_t) (sizeof(idle_source) - 1),
		(int16_t) (sizeof(end_source) - 1),
		(int16_t) (sizeof(endgame_source) - 1),
		(int16_t) (sizeof(lock_source) - 1),
		(int16_t) (sizeof(restart_source) - 1),
		(int16_t) (sizeof(go_source) - 1),
		(int16_t) (sizeof(try_source) - 1)
	};
	int16_t output_index = 0;
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	reset_oop_flow_case(sources[0], source_lengths[0], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(1, position, output_index);
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(2, position, output_index);

	reset_oop_flow_case(sources[1], source_lengths[1], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(3, position, output_index);

	reset_oop_flow_case(sources[2], source_lengths[2], ZOO_E_EMPTY);
	position = 0;
	test_state.world.info.health = 10;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(4, position, output_index);

	reset_oop_flow_case(sources[3], source_lengths[3], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(5, position, output_index);
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(6, position, output_index);

	reset_oop_flow_case(sources[4], source_lengths[4], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(7, position, output_index);

	reset_oop_flow_case(sources[5], source_lengths[5], ZOO_E_BOARD_EDGE);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(8, position, output_index);

	reset_oop_flow_case(sources[5], source_lengths[5], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(9, position, output_index);

	reset_oop_flow_case(sources[6], source_lengths[6], ZOO_E_BOARD_EDGE);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(10, position, output_index);

	reset_oop_flow_case(sources[6], source_lengths[6], ZOO_E_EMPTY);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flow probe");
	output_index = capture_oop_flow_output(11, position, output_index);

	return output_index == 11 ? 0 : 1;
}

int32_t run_oop_flow_control_checks(void) {
	static char idle_source[] = "#IDLE\r#CYCLE 7\r";
	static char end_source[] = "#END\r#CYCLE 9\r";
	static char endgame_source[] = "#ENDGAME\r#CYCLE 7\r";
	static char lock_source[] = "#LOCK\r#IDLE\r#UNLOCK\r#END\r";
	static char restart_source[] = "#RESTART\r#CYCLE 9\r";
	static char go_source[] = "#GO EAST #CYCLE 5\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = idle_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(idle_source) - 1);
	test_state.board.stats[1].cycle = 3;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].cycle != 3 || position != 6) return 1;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].cycle != 7) return 2;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = end_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(end_source) - 1);
	test_state.board.stats[1].cycle = 3;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].cycle != 3 || position != -1) return 3;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = endgame_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(endgame_source) - 1);
	test_state.board.stats[1].cycle = 3;
	test_state.world.info.health = 10;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.world.info.health != 0 || test_state.board.stats[1].cycle != 7) return 4;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = lock_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(lock_source) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].p2 != 1) return 5;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].p2 != 0 || position != -1) return 6;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = restart_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(restart_source) - 1);
	test_state.board.stats[1].cycle = 3;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (position != 0 || test_state.board.stats[1].cycle != 3) return 7;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = go_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(go_source) - 1);
	test_state.board.tiles[11][10].element = ZOO_E_BOARD_EDGE;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (position != 0 || test_state.board.stats[1].cycle != 0) return 8;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = go_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(go_source) - 1);
	test_state.board.tiles[11][10].element = ZOO_E_EMPTY;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (position != (int16_t) (sizeof(go_source) - 1) || test_state.board.stats[1].cycle != 0) return 9;

	return 0;
}

int32_t run_oop_object_command_checks(void) {
	static char become_source[] = "#BECOME SOLID\r";
	static char die_source[] = "#DIE\r";
	static char put_source[] = "#PUT N SOLID\r#END\r";
	static char put_edge_source[] = "#PUT N SOLID\r#END\r";
	static char change_source[] = "#CHANGE EMPTY SOLID\r";
	static char char_source[] = "#CHAR 65\r#END\r";
	static char walk_source[] = "#WALK WEST\r#END\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = become_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(become_source) - 1);
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	test_state.board.tiles[10][10].color = 0x2A;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.tiles[10][10].element != ZOO_E_SOLID) return 1;
	if (test_state.board.tiles[10][10].color != 0x2A) return 2;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = die_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(die_source) - 1);
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.tiles[10][10].element != ZOO_E_EMPTY) return 3;
	if (test_state.board.tiles[10][10].color != 0x70) return 4;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = put_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(put_source) - 1);
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	test_state.board.tiles[10][9].element = ZOO_E_EMPTY;
	test_state.board.tiles[10][9].color = 0x2A;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.tiles[10][9].element != ZOO_E_SOLID) return 5;
	if (test_state.board.tiles[10][9].color != 0x2A) return 6;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 1;
	test_state.board.stats[1].data = put_edge_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(put_edge_source) - 1);
	test_state.board.tiles[10][1].element = ZOO_E_OBJECT;
	test_state.board.tiles[10][0].element = ZOO_E_EMPTY;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.tiles[10][0].element != ZOO_E_EMPTY) return 7;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = change_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(change_source) - 1);
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.tiles[1][1].element != ZOO_E_SOLID) return 8;
	if (test_state.board.tiles[60][25].element != ZOO_E_SOLID) return 9;
	if (test_state.board.tiles[10][10].element != ZOO_E_OBJECT) return 10;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = char_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(char_source) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].p1 != 65) return 11;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = walk_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(walk_source) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.board.stats[1].step_x != -1 || test_state.board.stats[1].step_y != 0) return 12;

	return 0;
}

uint32_t oop_char_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_char_outputs;
}

int32_t run_oop_char_capture_checks(void) {
	static const char *scripts[8] = {
		"#CHAR -32768 #CYCLE 5\r",
		"#CHAR -1 #CYCLE 5\r",
		"#CHAR 0 #CYCLE 5\r",
		"#CHAR 1 #CYCLE 5\r",
		"#CHAR 127 #CYCLE 5\r",
		"#CHAR 255 #CYCLE 5\r",
		"#CHAR 256 #CYCLE 5\r",
		"#CHAR 32767 #CYCLE 5\r"
	};
	int case_index;
	for (case_index = 0; case_index < 8; case_index++) {
		int16_t position = 0;
		int output_index = case_index * 4;
		memset(&test_state, 0, sizeof(test_state));
		test_state.board.stat_count = 1;
		test_state.board.stats[1].x = 10;
		test_state.board.stats[1].y = 10;
		test_state.board.stats[1].cycle = 3;
		test_state.board.stats[1].p1 = 0xA5;
		test_state.board.stats[1].data = (char *) scripts[case_index];
		test_state.board.stats[1].data_len = (int16_t) strlen(scripts[case_index]);
		test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
		test_state.board.tiles[10][10].color = 0x0F;
		zoo_oop_execute(&test_state, 1, &position, "Char probe");
		oop_char_outputs[output_index] = (int16_t) (case_index + 1);
		oop_char_outputs[output_index + 1] = test_state.board.stats[1].p1;
		oop_char_outputs[output_index + 2] = test_state.board.stats[1].cycle;
		oop_char_outputs[output_index + 3] = position;
	}
	return 0;
}

uint32_t oop_put_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_put_outputs;
}

int32_t run_oop_put_capture_checks(void) {
	static const char *scripts[8] = {
		"#PUT NORTH SOLID\r#END\r",
		"#PUT NORTH SOLID\r#END\r",
		"#PUT SOUTH SOLID\r#END\r",
		"#PUT SOUTH SOLID\r#END\r",
		"#PUT EAST SOLID\r#END\r",
		"#PUT EAST SOLID\r#END\r",
		"#PUT WEST SOLID\r#END\r",
		"#PUT WEST SOLID\r#END\r"
	};
	static const int16_t source_x[8] = {10, 10, 10, 10, 59, 60, 1, 2};
	static const int16_t source_y[8] = {10, 1, 23, 24, 10, 10, 10, 10};
	static const int16_t delta_x[8] = {0, 0, 0, 0, 1, 1, -1, -1};
	static const int16_t delta_y[8] = {-1, -1, 1, 1, 0, 0, 0, 0};
	int case_index;
	for (case_index = 0; case_index < 8; case_index++) {
		int16_t position = 0;
		int16_t target_x = source_x[case_index] + delta_x[case_index];
		int16_t target_y = source_y[case_index] + delta_y[case_index];
		int16_t observed_x = source_x[case_index];
		int16_t observed_y = source_y[case_index];
		int output_index = case_index * 10;
		memset(&test_state, 0, sizeof(test_state));
		test_state.board.stat_count = 1;
		test_state.board.stats[1].x = source_x[case_index];
		test_state.board.stats[1].y = source_y[case_index];
		test_state.board.stats[1].cycle = 3;
		test_state.board.stats[1].data_pos = 0;
		test_state.board.stats[1].data = (char *) scripts[case_index];
		test_state.board.stats[1].data_len = (int16_t) strlen(scripts[case_index]);
		test_state.board.tiles[source_x[case_index]][source_y[case_index]].element = ZOO_E_OBJECT;
		test_state.board.tiles[source_x[case_index]][source_y[case_index]].color = 0x0F;
		if (target_x >= 0 && target_x <= ZOO_BOARD_WIDTH
			&& target_y >= 0 && target_y < ZOO_BOARD_HEIGHT) {
			observed_x = target_x;
			observed_y = target_y;
			test_state.board.tiles[target_x][target_y].element = ZOO_E_EMPTY;
			test_state.board.tiles[target_x][target_y].color = 0x2A;
		}
		zoo_oop_execute(&test_state, 1, &position, "PUT edge probe");
		oop_put_outputs[output_index] = (int16_t) (case_index + 1);
		oop_put_outputs[output_index + 1] = target_x;
		oop_put_outputs[output_index + 2] = target_y;
		oop_put_outputs[output_index + 3] = observed_x;
		oop_put_outputs[output_index + 4] = observed_y;
		oop_put_outputs[output_index + 5] = test_state.board.tiles[observed_x][observed_y].element;
		oop_put_outputs[output_index + 6] = test_state.board.tiles[observed_x][observed_y].color;
		oop_put_outputs[output_index + 7] = test_state.board.stats[1].cycle;
		oop_put_outputs[output_index + 8] = test_state.board.stats[1].data_pos;
		oop_put_outputs[output_index + 9] = position;
	}
	return 0;
}

uint32_t oop_change_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_change_outputs;
}

int32_t run_oop_change_capture_checks(void) {
	static char script[] = "#CHANGE EMPTY SOLID\r#END\r";
	int16_t position = 0;
	int16_t x;
	int16_t y;
	int output_index = 0;
	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].cycle = 3;
	test_state.board.stats[1].data_pos = 0;
	test_state.board.stats[1].data = script;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(script) - 1);
	for (y = 0; y <= ZOO_BOARD_HEIGHT; y++) {
		for (x = 0; x <= ZOO_BOARD_WIDTH; x++) {
			test_state.board.tiles[x][y].element = ZOO_E_EMPTY;
			test_state.board.tiles[x][y].color = (uint8_t) ((x * 31 + y * 17) & 0xFF);
		}
	}
	test_state.board.tiles[10][10].element = ZOO_E_OBJECT;
	test_state.board.tiles[10][10].color = 0x1F;
	test_state.board.tiles[11][10].element = ZOO_E_PLAYER;
	test_state.board.tiles[11][10].color = 0x0F;
	test_state.board.tiles[20][20].element = ZOO_E_SOLID;
	test_state.board.tiles[20][20].color = 0x3C;
	zoo_oop_execute(&test_state, 1, &position, "CHANGE map probe");
	for (y = 0; y <= ZOO_BOARD_HEIGHT; y++) {
		for (x = 0; x <= ZOO_BOARD_WIDTH; x++) {
			oop_change_outputs[output_index++] = test_state.board.tiles[x][y].element;
			oop_change_outputs[output_index++] = test_state.board.tiles[x][y].color;
		}
	}
	return 0;
}

int32_t run_oop_flag_and_send_command_checks(void) {
	static char set_source[] = "#SET ALPHA\r#END\r";
	static char clear_source[] = "#CLEAR ALPHA\r#END\r";
	static char send_source[] = "#SEND TARGET\r#CYCLE 7\r#END\r:TARGET\r#CYCLE 9\r#END\r";
	static char cross_send_source[] = "#SEND VILLAIN:HELLO\r#CYCLE 5\r#END\r";
	static char cross_target_source[] = "@VILLAIN\r:HELLO\r#CYCLE 9\r#END\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = set_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(set_source) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (zoo_flag_get_id(&test_state, "ALPHA") < 0) return 1;

	memset(&test_state, 0, sizeof(test_state));
	zoo_flag_set(&test_state, "ALPHA");
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = clear_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(clear_source) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (zoo_flag_get_id(&test_state, "ALPHA") >= 0) return 2;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = send_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(send_source) - 1);
	test_state.board.stats[1].data_pos = 0;
	zoo_oop_execute(&test_state, 1, &test_state.board.stats[1].data_pos, "Test");
	if (test_state.board.stats[1].cycle != 9) return 3;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 2;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = cross_send_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(cross_send_source) - 1);
	test_state.board.stats[1].data_pos = 0;
	test_state.board.stats[2].x = 20;
	test_state.board.stats[2].y = 10;
	test_state.board.stats[2].data = cross_target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(cross_target_source) - 1);
	test_state.board.stats[2].data_pos = 3;
	test_state.board.stats[2].p2 = 1;
	zoo_oop_execute(&test_state, 1, &test_state.board.stats[1].data_pos, "Test");
	if (test_state.board.stats[1].cycle != 5 || test_state.board.stats[2].data_pos != 3) return 4;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 2;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = cross_send_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(cross_send_source) - 1);
	test_state.board.stats[1].data_pos = 0;
	test_state.board.stats[2].x = 20;
	test_state.board.stats[2].y = 10;
	test_state.board.stats[2].data = cross_target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(cross_target_source) - 1);
	test_state.board.stats[2].data_pos = 3;
	zoo_oop_execute(&test_state, 1, &test_state.board.stats[1].data_pos, "Test");
	if (test_state.board.stats[1].cycle != 5 || test_state.board.stats[2].data_pos != 8) return 5;
	zoo_oop_execute(&test_state, 2, &test_state.board.stats[2].data_pos, "Villain");
	if (test_state.board.stats[2].cycle != 9) return 6;

	return 0;
}

static void capture_oop_send_result(size_t *output_index, bool sent, int16_t data_pos) {
	uint16_t encoded = (uint16_t) data_pos;
	oop_send_capture_outputs[(*output_index)++] = sent ? 1 : 0;
	oop_send_capture_outputs[(*output_index)++] = (uint8_t) encoded;
	oop_send_capture_outputs[(*output_index)++] = (uint8_t) (encoded >> 8);
}

static void capture_oop_send_i16(size_t *output_index, int16_t value) {
	uint16_t encoded = (uint16_t) value;
	oop_send_capture_outputs[(*output_index)++] = (uint8_t) encoded;
	oop_send_capture_outputs[(*output_index)++] = (uint8_t) (encoded >> 8);
}

uint32_t oop_send_capture_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_send_capture_outputs;
}

int32_t run_oop_send_capture_checks(void) {
	static char self_source[] = "@SENDER\r:GO\r#END\r";
	static char sender_source[] = "@SENDER\r";
	static char locked_target_source[] = "@VILLAIN\r:HELLO\r";
	static char open_target_source[] = "@villain\r:hello\r";
	static char self_dispatch_source[] = "#SEND GO\r#CYCLE 13\r#END\r:GO\r#CYCLE 17\r#END\r";
	static char cross_dispatch_source[] = "#SEND VILLAIN:HELLO\r#CYCLE 5\r#END\r";
	static char locked_dispatch_target_source[] = "@VILLAIN\r:HELLO\r#CYCLE 3\r#END\r";
	static char open_dispatch_target_source[] = "@villain\r:hello\r#CYCLE 9\r#END\r";
	bool sent;
	size_t output_index = 0;
	int16_t target_start_position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].data = self_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(self_source) - 1);
	test_state.board.stats[1].p2 = 1;
	sent = zoo_oop_send(&test_state, 1, "GO", false);
	if (!sent || test_state.board.stats[1].data_pos != 7) return 1;
	capture_oop_send_result(&output_index, sent, test_state.board.stats[1].data_pos);

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].data = self_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(self_source) - 1);
	test_state.board.stats[1].p2 = 1;
	sent = zoo_oop_send(&test_state, -1, "GO", false);
	if (sent || test_state.board.stats[1].data_pos != 0) return 2;
	capture_oop_send_result(&output_index, sent, test_state.board.stats[1].data_pos);

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 3;
	test_state.board.stats[1].data = sender_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(sender_source) - 1);
	test_state.board.stats[2].data = locked_target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(locked_target_source) - 1);
	test_state.board.stats[2].p2 = 1;
	test_state.board.stats[3].data = open_target_source;
	test_state.board.stats[3].data_len = (int16_t) (sizeof(open_target_source) - 1);
	sent = zoo_oop_send(&test_state, 1, "VILLAIN:HELLO", false);
	if (sent || test_state.board.stats[2].data_pos != 0 || test_state.board.stats[3].data_pos != 8) return 3;
	capture_oop_send_result(&output_index, sent, test_state.board.stats[2].data_pos);
	{
		uint16_t encoded = (uint16_t) test_state.board.stats[3].data_pos;
		oop_send_capture_outputs[output_index++] = (uint8_t) encoded;
		oop_send_capture_outputs[output_index++] = (uint8_t) (encoded >> 8);
	}

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].data = self_dispatch_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(self_dispatch_source) - 1);
	test_state.board.stats[1].p2 = 1;
	zoo_oop_execute(&test_state, 1, &test_state.board.stats[1].data_pos, "Self send dispatch");
	if (test_state.board.stats[1].cycle != 17) return 4;
	capture_oop_send_i16(&output_index, test_state.board.stats[1].cycle);

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 3;
	test_state.board.stats[1].data = cross_dispatch_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(cross_dispatch_source) - 1);
	test_state.board.stats[2].data = locked_dispatch_target_source;
	test_state.board.stats[2].data_len = (int16_t) (sizeof(locked_dispatch_target_source) - 1);
	test_state.board.stats[2].p2 = 1;
	test_state.board.stats[3].data = open_dispatch_target_source;
	test_state.board.stats[3].data_len = (int16_t) (sizeof(open_dispatch_target_source) - 1);
	zoo_oop_execute(&test_state, 1, &test_state.board.stats[1].data_pos, "Cross-object send dispatch");
	if (test_state.board.stats[1].cycle != 5
		|| test_state.board.stats[2].data_pos != 0
		|| test_state.board.stats[3].data_pos != 8) return 5;
	target_start_position = test_state.board.stats[3].data_pos;
	zoo_oop_execute(&test_state, 3, &test_state.board.stats[3].data_pos, "Villain target");
	if (test_state.board.stats[3].cycle != 9) return 6;
	capture_oop_send_i16(&output_index, test_state.board.stats[1].cycle);
	capture_oop_send_i16(&output_index, test_state.board.stats[2].data_pos);
	capture_oop_send_i16(&output_index, target_start_position);
	capture_oop_send_i16(&output_index, test_state.board.stats[3].data_pos);
	capture_oop_send_i16(&output_index, test_state.board.stats[3].cycle);

	if (output_index != sizeof(oop_send_capture_outputs)) return 7;
	return 0;
}

static void init_oop_flag_slot(int slot, const char *name, uint8_t marker) {
	size_t length = strlen(name);
	if (length > ZOO_LEN_FLAG) length = ZOO_LEN_FLAG;
	memset(test_state.world.info.io_flag_data[slot], marker, ZOO_LEN_FLAG);
	memcpy(test_state.world.info.io_flag_data[slot], name, length);
	test_state.world.info.io_flag_lengths[slot] = (uint8_t) length;
	test_state.world.info.io_flag_valid[slot] = true;
	memset(test_state.world.info.flags[slot], 0, sizeof(test_state.world.info.flags[slot]));
	memcpy(test_state.world.info.flags[slot], name, length);
}

static void capture_oop_flag_map(size_t *output_index) {
	int slot;
	int byte_index;
	for (slot = 0; slot < ZOO_MAX_FLAG; slot++) {
		oop_flag_capture_outputs[(*output_index)++] = test_state.world.info.io_flag_lengths[slot];
		for (byte_index = 0; byte_index < ZOO_LEN_FLAG; byte_index++) {
			oop_flag_capture_outputs[(*output_index)++] = test_state.world.info.io_flag_data[slot][byte_index];
		}
	}
}

static void capture_oop_flag_id(const char *name, size_t *output_index) {
	uint16_t value = (uint16_t) zoo_flag_get_id(&test_state, name);
	oop_flag_capture_outputs[(*output_index)++] = (uint8_t) value;
	oop_flag_capture_outputs[(*output_index)++] = (uint8_t) (value >> 8);
}

uint32_t oop_flag_capture_output_ptr(void) {
	return (uint32_t) (uintptr_t) oop_flag_capture_outputs;
}

int32_t run_oop_flag_state_capture_checks(void) {
	static char duplicate_script[] = "#CLEAR DUP\r#CLEAR DUP\r#SET BETA\r#SET alpha\r#END\r";
	static char full_script[] = "#SET OVERFLOW\r#END\r";
	static const char *initial_names[4] = {"ALPHA", "DUP", "DUP", "TAIL"};
	static const char *full_names[ZOO_MAX_FLAG] = {
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10"
	};
	int slot;
	int16_t position;
	size_t output_index = 0;
	memset(&test_state, 0, sizeof(test_state));
	for (slot = 0; slot < ZOO_MAX_FLAG; slot++)
		init_oop_flag_slot(slot, "", (uint8_t) (0xA1 + slot));
	for (slot = 0; slot < 4; slot++)
		init_oop_flag_slot(slot, initial_names[slot], (uint8_t) (0xA1 + slot));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].cycle = 3;
	test_state.board.stats[1].data = duplicate_script;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(duplicate_script) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Flag map probe");
	if (test_state.board.stats[1].data_pos < 0 || test_state.board.info.message[0] != '\0') return 1;
	capture_oop_flag_map(&output_index);
	capture_oop_flag_id("ALPHA", &output_index);
	capture_oop_flag_id("DUP", &output_index);
	capture_oop_flag_id("BETA", &output_index);
	capture_oop_flag_id("MISSING", &output_index);

	memset(&test_state, 0, sizeof(test_state));
	for (slot = 0; slot < ZOO_MAX_FLAG; slot++)
		init_oop_flag_slot(slot, full_names[slot], (uint8_t) (0xB1 + slot));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].cycle = 3;
	test_state.board.stats[1].data = full_script;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(full_script) - 1);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Full flag table probe");
	if (test_state.board.stats[1].data_pos < 0 || test_state.board.info.message[0] != '\0') return 2;
	capture_oop_flag_map(&output_index);
	capture_oop_flag_id("F1", &output_index);
	capture_oop_flag_id("F10", &output_index);
	capture_oop_flag_id("OVERFLOW", &output_index);
	if (output_index != sizeof(oop_flag_capture_outputs)) return 3;
	return 0;
}

int32_t run_oop_combat_and_play_checks(void) {
	static char shoot_source[] = "#SHOOT NORTH\r#CYCLE 5\r";
	static char shoot_blocked_source[] = "#SHOOT NORTH\r";
	static char throwstar_source[] = "#THROWSTAR FLOW\r#CYCLE 9\r";
	static char play_source[] = "#PLAY CDE\r#CYCLE 7\r#END\r";
	int16_t position;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = shoot_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(shoot_source) - 1);
	zoo_sound_state_init(&test_state.sound);
	shoot_call_count = 0;
	shoot_result = true;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (shoot_call_count != 1 || shoot_element != ZOO_E_BULLET) return 1;
	if (shoot_x != 10 || shoot_y != 10 || shoot_dx != 0 || shoot_dy != -1 || shoot_param1 != 1) return 2;
	if (test_state.sound.current_priority != 2 || test_state.sound.buffer_len != 4) return 3;
	if (test_state.sound.buffer[0] != 0x30 || test_state.sound.buffer[1] != 1
		|| test_state.sound.buffer[2] != 0x26 || test_state.sound.buffer[3] != 1) return 4;
	if (test_state.board.stats[1].cycle != 0) return 5;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = shoot_blocked_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(shoot_blocked_source) - 1);
	zoo_sound_state_init(&test_state.sound);
	shoot_call_count = 0;
	shoot_result = false;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (shoot_call_count != 1 || shoot_element != ZOO_E_BULLET) return 6;
	if (test_state.sound.buffer_len != 0 || test_state.board.stats[1].cycle != 0) return 7;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].step_x = -1;
	test_state.board.stats[1].step_y = 1;
	test_state.board.stats[1].data = throwstar_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(throwstar_source) - 1);
	zoo_sound_state_init(&test_state.sound);
	shoot_call_count = 0;
	shoot_result = true;
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (shoot_call_count != 1 || shoot_element != ZOO_E_STAR) return 8;
	if (shoot_x != 10 || shoot_y != 10 || shoot_dx != -1 || shoot_dy != 1 || shoot_param1 != 1) return 9;
	if (test_state.sound.buffer_len != 0 || test_state.board.stats[1].cycle != 0) return 10;

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stat_count = 1;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].data = play_source;
	test_state.board.stats[1].data_len = (int16_t) (sizeof(play_source) - 1);
	zoo_sound_state_init(&test_state.sound);
	position = 0;
	zoo_oop_execute(&test_state, 1, &position, "Test");
	if (test_state.sound.current_priority != -1 || test_state.sound.buffer_len != 6) return 11;
	if (test_state.sound.buffer[0] != 0x30 || test_state.sound.buffer[1] != 1
		|| test_state.sound.buffer[2] != 0x32 || test_state.sound.buffer[3] != 1
		|| test_state.sound.buffer[4] != 0x34 || test_state.sound.buffer[5] != 1) return 12;
	if (test_state.board.stats[1].cycle != 7) return 13;

	return 0;
}

static int run_oop_direction_case(const char *source, int16_t random_value,
	int expected_random_count, bool expected_valid, int16_t expected_dx, int16_t expected_dy) {
	int16_t position = 0;
	int16_t dx = 99;
	int16_t dy = 99;
	bool valid;

	memset(&test_state, 0, sizeof(test_state));
	memset(direction_random_values, 0, sizeof(direction_random_values));
	direction_random_count = 0;
	direction_random_values[0] = random_value;
	test_state.func_random = test_direction_random;
	test_state.board.stats[1].x = 10;
	test_state.board.stats[1].y = 10;
	test_state.board.stats[1].step_x = -1;
	test_state.board.stats[1].step_y = 1;
	test_state.board.stats[1].data = (char *) source;
	test_state.board.stats[1].data_len = (int16_t) strlen(source);
	zoo_oop_read_word(&test_state, 1, &position);
	valid = zoo_oop_parse_direction(&test_state, 1, &position, &dx, &dy);
	if (valid != expected_valid) return 1;
	if (dx != expected_dx || dy != expected_dy) return 2;
	if (direction_random_count != expected_random_count) return 3;
	return 0;
}

int run_oop_direction_parser_checks(void) {
	direction_rnd_dx = 1;
	direction_rnd_dy = -1;
	direction_seek_dx = -1;
	direction_seek_dy = 1;
	if (run_oop_direction_case("N", 0, 0, true, 0, -1) != 0) return 1;
	if (run_oop_direction_case("SOUTH", 0, 0, true, 0, 1) != 0) return 2;
	if (run_oop_direction_case("E", 0, 0, true, 1, 0) != 0) return 3;
	if (run_oop_direction_case("WEST", 0, 0, true, -1, 0) != 0) return 4;
	if (run_oop_direction_case("I", 0, 0, true, 0, 0) != 0) return 5;
	if (run_oop_direction_case("FLOW", 0, 0, true, -1, 1) != 0) return 6;
	if (run_oop_direction_case("SEEK", 0, 0, true, -1, 1) != 0) return 7;
	if (run_oop_direction_case("RND", 0, 0, true, 1, -1) != 0) return 8;
	if (run_oop_direction_case("CW NORTH", 0, 0, true, 1, 0) != 0) return 9;
	if (run_oop_direction_case("CCW NORTH", 0, 0, true, -1, 0) != 0) return 10;
	if (run_oop_direction_case("OPP WEST", 0, 0, true, 1, 0) != 0) return 11;
	if (run_oop_direction_case("RNDP NORTH", 0, 1, true, 1, 0) != 0) return 12;
	if (run_oop_direction_case("RNDP NORTH", 1, 1, true, -1, 0) != 0) return 13;
	if (run_oop_direction_case("RNDNS", 0, 1, true, 0, -1) != 0) return 14;
	if (run_oop_direction_case("RNDNS", 1, 1, true, 0, 1) != 0) return 15;
	if (run_oop_direction_case("RNDNE", 0, 1, true, 0, -1) != 0) return 16;
	if (run_oop_direction_case("RNDNE", 1, 1, true, 1, 0) != 0) return 17;
	if (run_oop_direction_case("INVALID", 0, 0, false, 0, 0) != 0) return 18;
	return 0;
}

static int run_oop_tile_case(const char *source, bool expected_valid, uint8_t expected_element,
	uint8_t expected_color, bool preserve_element) {
	int16_t position = 0;
	bool valid;
	zoo_tile tile = {.element = ZOO_E_BOARD_EDGE, .color = 0xFF};

	memset(&test_state, 0, sizeof(test_state));
	test_state.board.stats[1].data = (char *) source;
	test_state.board.stats[1].data_len = (int16_t) strlen(source);
	valid = zoo_oop_parse_tile(&test_state, 1, &position, &tile);
	if (valid != expected_valid) return 1;
	if (tile.color != expected_color) return 2;
	if (preserve_element) {
		if (tile.element != ZOO_E_BOARD_EDGE) return 3;
	} else if (tile.element != expected_element) {
		return 4;
	}
	return 0;
}

int run_oop_tile_parser_checks(void) {
	if (run_oop_tile_case("EMPTY", true, ZOO_E_EMPTY, 0, false) != 0) return 1;
	if (run_oop_tile_case("SOLID", true, ZOO_E_SOLID, 0, false) != 0) return 2;
	if (run_oop_tile_case("SPINNINGGUN", true, ZOO_E_SPINNING_GUN, 0, false) != 0) return 3;
	if (run_oop_tile_case("blue empty", true, ZOO_E_EMPTY, 9, false) != 0) return 4;
	if (run_oop_tile_case("GREEN EMPTY", true, ZOO_E_EMPTY, 10, false) != 0) return 5;
	if (run_oop_tile_case("CYAN EMPTY", true, ZOO_E_EMPTY, 11, false) != 0) return 6;
	if (run_oop_tile_case("RED EMPTY", true, ZOO_E_EMPTY, 12, false) != 0) return 7;
	if (run_oop_tile_case("PURPLE SOLID", true, ZOO_E_SOLID, 13, false) != 0) return 8;
	if (run_oop_tile_case("YELLOW EMPTY", true, ZOO_E_EMPTY, 14, false) != 0) return 9;
	if (run_oop_tile_case("WHITE EMPTY", true, ZOO_E_EMPTY, 15, false) != 0) return 10;
	if (run_oop_tile_case("UNKNOWN", false, ZOO_E_BOARD_EDGE, 0, true) != 0) return 11;
	if (run_oop_tile_case("YELLOW UNKNOWN", false, ZOO_E_BOARD_EDGE, 14, true) != 0) return 12;
	return 0;
}

int32_t run_oop_place_tile_checks(void) {
	int color;
	for (color = 0; color <= 255; color++) {
		zoo_tile door = {ZOO_E_DOOR, 0};
		memset(&test_state, 0, sizeof(test_state));
		test_state.board.tiles[1][1].element = ZOO_E_EMPTY;
		test_state.board.tiles[1][1].color = (uint8_t) color;
		zoo_oop_place_tile(&test_state, 1, 1, door);
		if (test_state.board.tiles[1][1].element != ZOO_E_DOOR) return color + 1;
		if (test_state.board.tiles[1][1].color
			!= (uint8_t) ((((color == 0 ? 0x0F : color) - 8) * 0x10) + 0x0F)) {
			return color + 17;
		}
	}
	return 0;
}

int32_t run_oop_line_reader_checks(void) {
	char output[256];
	int line_length;
	for (line_length = 0; line_length <= 255; line_length++) {
		int16_t position = 0;
		memset(&test_state, 0, sizeof(test_state));
		memset(oop_line_input, 'A', (size_t) line_length);
		oop_line_input[line_length] = '\r';
		test_state.board.stats[1].data = (char *) oop_line_input;
		test_state.board.stats[1].data_len = (int16_t) (line_length + 1);
		memset(output, 0xCC, sizeof(output));
		zoo_oop_read_line_to_end(&test_state, 1, &position, output, 255);
		if ((int) strlen(output) != line_length) return line_length + 1;
		if (position != line_length + 1) return line_length + 257;
		if (output[line_length] != '\0') return line_length + 513;
	}

	// Turbo Pascal 5 truncates string concatenation at 255 bytes; the OOP reader
	// keeps consuming input through CR after its string has reached that limit.
	{
		int16_t position = 0;
		memset(&test_state, 0, sizeof(test_state));
		memset(oop_line_input, 'B', 300);
		oop_line_input[300] = '\r';
		test_state.board.stats[1].data = (char *) oop_line_input;
		test_state.board.stats[1].data_len = 301;
		memset(output, 0xCC, sizeof(output));
		zoo_oop_read_line_to_end(&test_state, 1, &position, output, 255);
		if (strlen(output) != 255) return 769;
		if (position != 301) return 770;
	}
	return 0;
}
