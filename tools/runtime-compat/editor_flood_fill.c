#include <stddef.h>
#include <stdint.h>

#include "zoo.h"
#include "zoo_editor.h"

#define JNIEXPORT
#define JNICALL
#define JNI_TRUE 1
#define JNI_FALSE 0
typedef int32_t jint;
typedef uint8_t jboolean;
typedef void JNIEnv;
typedef void *jclass;

static uint8_t editor_allocation_pool[1024];
static size_t editor_allocation_pool_used;
static size_t editor_free_count;
static uint8_t editor_board_data_heap[1024];

void *memcpy(void *destination, const void *source, size_t count) {
	unsigned char *to = (unsigned char *) destination;
	const unsigned char *from = (const unsigned char *) source;
	for (size_t i = 0; i < count; i++) to[i] = from[i];
	return destination;
}

void *malloc(size_t size) {
	if (size > sizeof(editor_allocation_pool) - editor_allocation_pool_used) return NULL;
	void *result = editor_allocation_pool + editor_allocation_pool_used;
	editor_allocation_pool_used += size;
	return result;
}

void free(void *pointer) {
	if (pointer != NULL) editor_free_count++;
}

char *zoo_board_data_heap_alloc(zoo_board *board, uint32_t size, uint32_t *offset) {
	if (board->data_heap == NULL || size > board->data_heap_capacity - board->data_heap_used) return NULL;
	*offset = board->data_heap_used;
	char *result = board->data_heap + board->data_heap_used;
	board->data_heap_used += size;
	return result;
}

void zoo_board_data_heap_release(zoo_board *board, char *data, uint32_t reuse_order) {
	(void) board;
	(void) data;
	(void) reuse_order;
}

void *memset(void *destination, int value, size_t count) {
	unsigned char *bytes = (unsigned char *) destination;
	for (size_t i = 0; i < count; i++) bytes[i] = (unsigned char) value;
	return destination;
}

static zoo_state game;
static bool ready;
static bool editor_active;
static bool editor_was_modified;
static bool editor_clipboard_valid;
static bool editor_clipboard_has_stat;
static zoo_tile editor_clipboard_tile;
static zoo_stat editor_clipboard_stat;
static uint8_t editor_placement_outputs[68];
static char editor_paste_script_data[4] = {'A', '\r', '\0', 'Z'};
static uint8_t editor_fill_snapshot[ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT];

typedef struct {
	uint8_t p1, p2, p3;
	int16_t step_x, step_y;
} editor_stat_setting;
static editor_stat_setting editor_stat_settings[ZOO_MAX_ELEMENT + 1];

const zoo_stat zoo_stat_template_default = {0};
const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
	[ZOO_E_EMPTY] = {.color = 0x70, .placeable_on_top = false, .walkable = true, .cycle = -1},
	[ZOO_E_PLAYER] = {.color = 0x1F, .cycle = 1},
	[ZOO_E_WATER] = {.color = 0x17, .cycle = -1, .walkable = true},
	[ZOO_E_SOLID] = {.color = 0x0F, .cycle = -1},
	[ZOO_E_NORMAL] = {.color = 0x07, .cycle = -1},
	[ZOO_E_BREAKABLE] = {.color = 0x0F, .cycle = -1},
	[ZOO_E_LINE] = {.color = 0x0F, .cycle = -1},
};

void zoo_board_draw_tile(zoo_state *state, int16_t x, int16_t y) {
	(void) state;
	(void) x;
	(void) y;
}

int16_t zoo_stat_get_id(zoo_state *state, int16_t x, int16_t y) {
	for (int16_t stat_id = 0; stat_id <= state->board.stat_count; stat_id++) {
		if (state->board.stats[stat_id].x == x && state->board.stats[stat_id].y == y) return stat_id;
	}
	return -1;
}

void zoo_stat_remove(zoo_state *state, int16_t stat_id) {
	(void) state;
	(void) stat_id;
}

void zoo_stat_move(zoo_state *state, int16_t stat_id, int16_t new_x, int16_t new_y) {
	(void) state;
	(void) stat_id;
	(void) new_x;
	(void) new_y;
}

void zoo_stat_add(zoo_state *state, int16_t tx, int16_t ty, uint8_t element,
		int16_t color, int16_t tcycle, const zoo_stat *stat_template);

#include "zoo_stat_add_production.inc"
#include "editor_placement_production.inc"
#include "editor_flood_fill_production.inc"

static void clear_state(void) {
	unsigned char *bytes = (unsigned char *) &game;
	for (size_t i = 0; i < sizeof(game); i++) bytes[i] = 0;
	ready = true;
	editor_active = true;
	editor_was_modified = false;
	editor_clipboard_valid = false;
	editor_clipboard_has_stat = false;
	for (int16_t x = 0; x <= ZOO_BOARD_WIDTH + 1; x++) {
		for (int16_t y = 0; y <= ZOO_BOARD_HEIGHT + 1; y++) {
			zoo_tile tile = {ZOO_E_BOARD_EDGE, 0};
			if (x >= 1 && x <= ZOO_BOARD_WIDTH && y >= 1 && y <= ZOO_BOARD_HEIGHT) {
				tile.element = ZOO_E_EMPTY;
			}
			if (x == 1 || x == ZOO_BOARD_WIDTH || y == 1 || y == ZOO_BOARD_HEIGHT) {
				tile.element = ZOO_E_NORMAL;
				tile.color = 0x0E;
			}
			game.board.tiles[x][y] = tile;
		}
	}
}

static void set_tile(int16_t x, int16_t y, uint8_t element, uint8_t color) {
	game.board.tiles[x][y].element = element;
	game.board.tiles[x][y].color = color;
}

static void snapshot_board(void) {
	int offset = 0;
	for (int16_t y = 1; y <= ZOO_BOARD_HEIGHT; y++) {
		for (int16_t x = 1; x <= ZOO_BOARD_WIDTH; x++) {
			zoo_tile tile = game.board.tiles[x][y];
			if (tile.element == ZOO_E_EMPTY) {
				editor_fill_snapshot[offset++] = '.';
			} else if (tile.element == ZOO_E_SOLID) {
				editor_fill_snapshot[offset++] = tile.color == 0x0E ? 'F' : 'S';
			} else if (tile.element == ZOO_E_PLAYER) {
				editor_fill_snapshot[offset++] = 'P';
			} else {
				editor_fill_snapshot[offset++] = '?';
			}
		}
	}
}

int32_t run_editor_fill_case(int32_t case_id) {
	int16_t start_x;
	int16_t start_y;
	clear_state();

	if (case_id == 0) {
		for (int16_t y = 1; y <= ZOO_BOARD_HEIGHT; y++)
			for (int16_t x = 1; x <= ZOO_BOARD_WIDTH; x++)
				set_tile(x, y, ZOO_E_SOLID, 0x0F);
		for (int16_t y = 11; y <= 13; y++)
			for (int16_t x = 29; x <= 31; x++)
				set_tile(x, y, ZOO_E_EMPTY, 0x0F);
		game.board.stats[0].x = 1;
		game.board.stats[0].y = 1;
		start_x = 30;
		start_y = 12;
	} else if (case_id == 1) {
		for (int16_t y = 1; y <= ZOO_BOARD_HEIGHT; y++)
			for (int16_t x = 1; x <= ZOO_BOARD_WIDTH; x++)
				set_tile(x, y, ZOO_E_SOLID, 0x0F);
		for (int16_t y = 1; y <= ZOO_BOARD_HEIGHT; y++) {
			if ((y % 2) == 1) {
				for (int16_t x = 1; x <= ZOO_BOARD_WIDTH; x++)
					game.board.tiles[x][y].element = ZOO_E_EMPTY;
			} else if (((y / 2) % 2) == 1) {
				game.board.tiles[ZOO_BOARD_WIDTH][y].element = ZOO_E_EMPTY;
			} else {
				game.board.tiles[1][y].element = ZOO_E_EMPTY;
			}
		}
		game.board.stats[0].x = 30;
		game.board.stats[0].y = 2;
		set_tile(30, 2, ZOO_E_PLAYER, 0x0F);
		start_x = 30;
		start_y = 1;
	} else if (case_id == 2) {
		for (int16_t y = 1; y <= ZOO_BOARD_HEIGHT; y++)
			for (int16_t x = 1; x <= ZOO_BOARD_WIDTH; x++)
				set_tile(x, y, ZOO_E_SOLID, 0x0F);
		game.board.stats[0].x = 1;
		game.board.stats[0].y = 1;
		set_tile(1, 1, ZOO_E_PLAYER, 0x0F);
		for (int16_t y = 4; y <= 20; y++)
			for (int16_t x = 20; x <= 36; x++)
				set_tile(x, y, ZOO_E_EMPTY, 0x0F);
		start_x = 28;
		start_y = 12;
	} else if (case_id == 3) {
		game.board.stats[0].x = 1;
		game.board.stats[0].y = 1;
		set_tile(1, 1, ZOO_E_PLAYER, 0x0F);
		for (int16_t y = 2; y < ZOO_BOARD_HEIGHT; y++)
			for (int16_t x = 2; x < ZOO_BOARD_WIDTH; x++)
				set_tile(x, y, ZOO_E_EMPTY, 0x0F);
		start_x = 30;
		start_y = 12;
	} else {
		return -1;
	}

	int32_t changed = Java_com_kerry_zzt_ZztNative_editorFloodFill(NULL, NULL,
		start_x, start_y, ZOO_E_SOLID, 0x0E, JNI_FALSE, JNI_TRUE);
	snapshot_board();
	return changed;
}

uint32_t editor_fill_snapshot_ptr(void) {
	return (uint32_t) (uintptr_t) editor_fill_snapshot;
}

int32_t editor_fill_tile_element(int32_t x, int32_t y) {
	if (x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) return -1;
	return game.board.tiles[x][y].element;
}

int32_t editor_fill_tile_color(int32_t x, int32_t y) {
	if (x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) return -1;
	return game.board.tiles[x][y].color;
}

static void capture_editor_placement(size_t offset, int result, int16_t x, int16_t y) {
	editor_placement_outputs[offset] = (uint8_t) result;
	editor_placement_outputs[offset + 1] = game.board.tiles[x][y].element;
	editor_placement_outputs[offset + 2] = game.board.tiles[x][y].color;
	editor_placement_outputs[offset + 3] = editor_was_modified ? 1 : 0;
	editor_placement_outputs[offset + 4] = (uint8_t) game.board.stat_count;
}

int32_t run_editor_placement_checks(void) {
	int result;
	size_t free_count_before_clear;
	zoo_stat source_template;
	clear_state();
	game.board.stats[0].x = 1;
	game.board.stats[0].y = 1;
	set_tile(30, 12, ZOO_E_SOLID, 0x0F);
	editor_was_modified = false;
	result = editor_place_pattern_element(30, 12, ZOO_E_NORMAL, 0x0E);
	capture_editor_placement(0, result, 30, 12);
	if (result != 0 || game.board.tiles[30][12].element != ZOO_E_NORMAL
		|| game.board.tiles[30][12].color != 0x0E || !editor_was_modified) return 1;

	game.board.stats[0].x = 31;
	game.board.stats[0].y = 12;
	set_tile(31, 12, ZOO_E_PLAYER, 0x0F);
	editor_was_modified = false;
	result = editor_place_pattern_element(31, 12, ZOO_E_NORMAL, 0x0E);
	capture_editor_placement(5, result, 31, 12);
	if (result != 1 || game.board.tiles[31][12].element != ZOO_E_PLAYER
		|| game.board.tiles[31][12].color != 0x0F || !editor_was_modified) return 2;

	game.board.stats[0].x = 1;
	game.board.stats[0].y = 1;
	set_tile(32, 12, ZOO_E_SOLID, 0x0F);
	editor_clipboard_has_stat = false;
	editor_clipboard_tile.element = ZOO_E_WATER;
	editor_clipboard_tile.color = 0x7F;
	editor_was_modified = false;
	result = editor_paste_tile(32, 12);
	capture_editor_placement(10, result, 32, 12);
	if (result != 0 || game.board.tiles[32][12].element != ZOO_E_WATER
		|| game.board.tiles[32][12].color != 0x7F || !editor_was_modified) return 3;

	clear_state();
	game.board.stat_count = ZOO_MAX_STAT;
	set_tile(34, 12, ZOO_E_SOLID, 0x0F);
	editor_clipboard_has_stat = true;
	editor_clipboard_tile.element = ZOO_E_WATER;
	editor_clipboard_tile.color = 0x17;
	editor_was_modified = false;
	result = editor_paste_tile(34, 12);
	capture_editor_placement(15, result, 34, 12);
	if (result != 1 || game.board.tiles[34][12].element != ZOO_E_SOLID
		|| game.board.tiles[34][12].color != 0x0F || editor_was_modified
		|| game.board.stat_count != ZOO_MAX_STAT) return 4;

	clear_state();
	editor_allocation_pool_used = 0;
	game.board.stats[0].x = 31;
	game.board.stats[0].y = 12;
	game.board.data_heap = (char *) editor_board_data_heap;
	game.board.data_heap_capacity = sizeof(editor_board_data_heap);
	set_tile(35, 12, ZOO_E_SOLID, 0x2F);
	editor_clipboard_has_stat = true;
	editor_clipboard_tile.element = ZOO_E_BOMB;
	editor_clipboard_tile.color = 0x17;
	editor_clipboard_stat.cycle = 9;
	editor_clipboard_stat.step_x = 2;
	editor_clipboard_stat.step_y = 3;
	editor_clipboard_stat.p1 = 77;
	editor_clipboard_stat.p2 = 88;
	editor_clipboard_stat.p3 = 99;
	editor_clipboard_stat.follower = -1;
	editor_clipboard_stat.leader = -1;
	editor_clipboard_stat.data = NULL;
	editor_clipboard_stat.data_len = 0;
	editor_clipboard_stat.data_pos = 3;
	editor_was_modified = false;
	result = editor_paste_tile(35, 12);
	capture_editor_placement(20, result, 35, 12);
	zoo_stat *pasted = &game.board.stats[1];
	editor_placement_outputs[25] = pasted->x;
	editor_placement_outputs[26] = pasted->y;
	editor_placement_outputs[27] = (uint8_t) pasted->cycle;
	editor_placement_outputs[28] = (uint8_t) pasted->step_x;
	editor_placement_outputs[29] = (uint8_t) pasted->step_y;
	editor_placement_outputs[30] = pasted->p1;
	editor_placement_outputs[31] = pasted->p2;
	editor_placement_outputs[32] = pasted->p3;
	editor_placement_outputs[33] = (uint8_t) pasted->follower;
	editor_placement_outputs[34] = (uint8_t) pasted->leader;
	editor_placement_outputs[35] = pasted->under.element;
	editor_placement_outputs[36] = pasted->under.color;
	editor_placement_outputs[37] = (uint8_t) pasted->data_len;
	editor_placement_outputs[38] = (uint8_t) pasted->data_pos;
	for (int i = 0; i < 4; i++) editor_placement_outputs[39 + i] = pasted->data != NULL
		? (uint8_t) pasted->data[i] : 0;
	editor_placement_outputs[43] = pasted->data_in_heap ? 1 : 0;
	if (result != 0 || game.board.stat_count != 1 || pasted->x != 35 || pasted->y != 12
		|| pasted->cycle != 9 || pasted->step_x != 2 || pasted->step_y != 3
		|| pasted->p1 != 77 || pasted->p2 != 88 || pasted->p3 != 99
		|| pasted->follower != -1 || pasted->leader != -1
		|| pasted->under.element != ZOO_E_EMPTY || pasted->under.color != 0x2F
		|| pasted->data_len != 0 || pasted->data_pos != 0 || pasted->data != NULL
		|| pasted->data_in_heap) return 5;

	clear_state();
	editor_allocation_pool_used = 0;
	game.board.stats[0].x = 1;
	game.board.stats[0].y = 1;
	game.board.data_heap = (char *) editor_board_data_heap;
	game.board.data_heap_capacity = sizeof(editor_board_data_heap);
	editor_paste_script_data[0] = 'A';
	memset(&source_template, 0, sizeof(source_template));
	source_template.cycle = 9;
	source_template.step_x = 2;
	source_template.step_y = 3;
	source_template.p1 = 77;
	source_template.p2 = 88;
	source_template.p3 = 99;
	source_template.follower = -1;
	source_template.leader = -1;
	source_template.data = editor_paste_script_data;
	source_template.data_len = sizeof(editor_paste_script_data);
	source_template.data_pos = 3;
	set_tile(31, 12, ZOO_E_SOLID, 0x2F);
	set_tile(36, 12, ZOO_E_SOLID, 0x2F);
	zoo_stat_add(&game, 31, 12, ZOO_E_BOMB, 0x17, source_template.cycle, &source_template);
	if (game.board.stat_count != 1 || !game.board.stats[1].data_in_heap
		|| game.board.stats[1].data == (char *) editor_paste_script_data
		|| game.board.stats[1].data_len != sizeof(editor_paste_script_data)) return 6;
	editor_was_modified = false;
	result = Java_com_kerry_zzt_ZztNative_editorCopyTile(NULL, NULL, 31, 12);
	if (result != 0 || !editor_clipboard_valid || !editor_clipboard_has_stat
		|| editor_clipboard_stat.data != game.board.stats[1].data
		|| editor_clipboard_stat.data_len != sizeof(editor_paste_script_data)
		|| editor_clipboard_stat.data == NULL
		|| editor_clipboard_stat.data[0] != 'A' || editor_clipboard_stat.data[1] != '\r'
		|| editor_clipboard_stat.data[2] != '\0' || editor_clipboard_stat.data[3] != 'Z') return 7;
	game.board.stats[1].data[0] = 'B';
	if (editor_clipboard_stat.data[0] != 'B') return 8;
	result = editor_paste_tile(36, 12);
	capture_editor_placement(44, result, 36, 12);
	pasted = &game.board.stats[2];
	editor_placement_outputs[49] = pasted->x;
	editor_placement_outputs[50] = pasted->y;
	editor_placement_outputs[51] = (uint8_t) pasted->cycle;
	editor_placement_outputs[52] = (uint8_t) pasted->step_x;
	editor_placement_outputs[53] = (uint8_t) pasted->step_y;
	editor_placement_outputs[54] = pasted->p1;
	editor_placement_outputs[55] = pasted->p2;
	editor_placement_outputs[56] = pasted->p3;
	editor_placement_outputs[57] = (uint8_t) pasted->follower;
	editor_placement_outputs[58] = (uint8_t) pasted->leader;
	editor_placement_outputs[59] = pasted->under.element;
	editor_placement_outputs[60] = pasted->under.color;
	editor_placement_outputs[61] = (uint8_t) pasted->data_len;
	editor_placement_outputs[62] = (uint8_t) pasted->data_pos;
	for (int i = 0; i < 4; i++) editor_placement_outputs[63 + i] = pasted->data != NULL
		? (uint8_t) pasted->data[i] : 0;
	editor_placement_outputs[67] = pasted->data_in_heap ? 1 : 0;
	if (result != 0 || game.board.stat_count != 2 || pasted->x != 36 || pasted->y != 12
		|| pasted->cycle != 9 || pasted->step_x != 2 || pasted->step_y != 3
		|| pasted->p1 != 77 || pasted->p2 != 88 || pasted->p3 != 99
		|| pasted->follower != -1 || pasted->leader != -1
		|| pasted->under.element != ZOO_E_EMPTY || pasted->under.color != 0x2F
		|| pasted->data_len != 4 || pasted->data_pos != 0 || !pasted->data_in_heap
		|| pasted->data == NULL || pasted->data_heap_offset != sizeof(editor_paste_script_data)
		|| pasted->data[0] != 'B' || pasted->data[1] != '\r'
		|| pasted->data[2] != '\0' || pasted->data[3] != 'Z') return 9;
	free_count_before_clear = editor_free_count;
	editor_clipboard_clear();
	if (editor_free_count != free_count_before_clear || editor_clipboard_valid
		|| editor_clipboard_has_stat || pasted->data[0] != 'B'
		|| pasted->data[1] != '\r' || pasted->data[2] != '\0'
		|| pasted->data[3] != 'Z') return 10;
	return 0;
}

uint32_t editor_placement_output_ptr(void) {
	return (uint32_t) (uintptr_t) editor_placement_outputs;
}


int32_t run_editor_fill_self_checks(void) {
	if (run_editor_fill_case(0) != 9) return 1;
	if (editor_fill_tile_element(30, 12) != ZOO_E_SOLID
		|| editor_fill_tile_color(30, 12) != 0x0E
		|| editor_fill_tile_element(28, 12) != ZOO_E_SOLID
		|| editor_fill_tile_color(28, 12) != 0x0F) return 2;
	if (run_editor_fill_case(1) != 792) return 3;
	if (run_editor_fill_case(2) != 289) return 4;
	if (run_editor_fill_case(3) != 1334) return 5;
	return 0;
}
