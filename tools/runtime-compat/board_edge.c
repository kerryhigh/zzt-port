#include <stddef.h>
#include "zoo_internal.h"

static zoo_state state;
static zoo_call call_storage;
static int transition_count;
static int board_enter_count;
static int board_change_count;
static int16_t destination_element;
static int16_t destination_board_id;
static int16_t destination_entry_x;
static int16_t destination_entry_y;
static int16_t source_player_x;
static int16_t source_player_y;
static bool destination_player_at_edge;
static int32_t board_edge_output[6][10];

static void clear_test_state(void) {
	unsigned char *bytes = (unsigned char *) &state;
	for (size_t i = 0; i < sizeof(state); i++) bytes[i] = 0;
	for (size_t i = 0; i < sizeof(call_storage); i++) ((unsigned char *) &call_storage)[i] = 0;
	transition_count = 0;
	board_enter_count = 0;
	board_change_count = 0;
}

static void capture_board_edge_row(int row) {
	int16_t x = state.board.stats[0].x;
	int16_t y = state.board.stats[0].y;
	board_edge_output[row][0] = state.world.info.current_board;
	board_edge_output[row][1] = x;
	board_edge_output[row][2] = y;
	board_edge_output[row][3] = state.board.tiles[x][y].element;
	board_edge_output[row][4] = state.input.delta_x;
	board_edge_output[row][5] = state.input.delta_y;
	board_edge_output[row][6] = state.board.info.start_player_x;
	board_edge_output[row][7] = state.board.info.start_player_y;
	board_edge_output[row][8] = state.world.info.board_time_sec;
	board_edge_output[row][9] = state.game_paused ? 1 : 0;
}

uint32_t board_edge_output_ptr(void) {
	return (uint32_t) (uintptr_t) board_edge_output;
}

int32_t board_edge_output_rows(void) {
	return 6;
}

int32_t board_edge_output_columns(void) {
	return 10;
}

zoo_call *zoo_call_push(zoo_call_stack *stack, zoo_call_type type, uint8_t call_state) {
	call_storage.type = type;
	call_storage.state = call_state;
	stack->call = &call_storage;
	return &call_storage;
}

void zoo_board_change(zoo_state *state, int16_t board_id) {
	board_change_count++;
	state->world.info.current_board = board_id;
	if (board_id == destination_board_id) {
		state->board.stats[0].x = destination_player_at_edge ? destination_entry_x : 30;
		state->board.stats[0].y = destination_player_at_edge ? destination_entry_y : 12;
		state->board.tiles[30][12].element = destination_player_at_edge
			? ZOO_E_EMPTY : ZOO_E_PLAYER;
		state->board.tiles[destination_entry_x][destination_entry_y].element =
			destination_player_at_edge ? ZOO_E_PLAYER : (uint8_t) destination_element;
	} else {
		state->board.stats[0].x = source_player_x;
		state->board.stats[0].y = source_player_y;
		state->board.tiles[source_player_x][source_player_y].element = ZOO_E_PLAYER;
	}
}

void zoo_board_draw_transition(zoo_state *state) {
	(void) state;
	transition_count++;
}

void zoo_board_enter(zoo_state *state) {
	board_enter_count++;
	state->board.info.start_player_x = state->board.stats[0].x;
	state->board.info.start_player_y = state->board.stats[0].y;
	state->world.info.board_time_sec = 0;
}

void zoo_stat_move(zoo_state *state, int16_t stat_id, int16_t new_x, int16_t new_y) {
	zoo_stat *stat = &state->board.stats[stat_id];
	zoo_tile player = state->board.tiles[stat->x][stat->y];
	state->board.tiles[stat->x][stat->y].element = ZOO_E_EMPTY;
	state->board.tiles[new_x][new_y] = player;
	stat->x = new_x;
	stat->y = new_y;
}

static void compat_default_touch(zoo_state *state, int16_t x, int16_t y,
	int16_t source_stat_id, int16_t *dx, int16_t *dy) {
	(void) state;
	(void) x;
	(void) y;
	(void) source_stat_id;
	(void) dx;
	(void) dy;
}

const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
	[ZOO_E_EMPTY] = {.walkable = true, .touch_func = compat_default_touch},
	[ZOO_E_PLAYER] = {.touch_func = compat_default_touch},
	[ZOO_E_SOLID] = {.touch_func = compat_default_touch}
};

#define CALL_STATE_GET_SELF uint8_t call_state = state->call_stack.state; state->call_stack.state = 0
#define CALL_SET_TOUCH_ARGS(c) \
	(c)->args.touch.x = x; \
	(c)->args.touch.y = y; \
	(c)->args.touch.source_stat_id = source_stat_id; \
	(c)->args.touch.dx = dx; \
	(c)->args.touch.dy = dy
#define CALL_OOM_CHECK(state, val) \
	if ((val) == NULL) { (state)->error_value = ZOO_ERROR_NOMEM; return; }

#include "board_edge_production.inc"

static int invoke_edge_touch(int destination, bool player_at_edge, int16_t target_board,
	int16_t source_x, int16_t source_y, int16_t dx, int16_t dy,
	int16_t target_x, int16_t target_y) {
	int16_t neighbor_id = dy == -1 ? 0 : dy == 1 ? 1 : dx == -1 ? 2 : 3;
	clear_test_state();
	destination_element = (int16_t) destination;
	destination_player_at_edge = player_at_edge;
	destination_board_id = target_board;
	destination_entry_x = target_x;
	destination_entry_y = target_y;
	source_player_x = source_x;
	source_player_y = source_y;
	state.world.info.current_board = 0;
	state.world.info.board_time_sec = 42;
	state.board.info.neighbor_boards[neighbor_id] = target_board;
	state.board.stats[0].x = source_x;
	state.board.stats[0].y = source_y;
	state.board.info.start_player_x = source_x;
	state.board.info.start_player_y = source_y;
	state.board.tiles[source_x][source_y].element = ZOO_E_PLAYER;
	state.input.delta_x = dx;
	state.input.delta_y = dy;

	zoo_e_board_edge_touch(&state, source_x + dx, source_y + dy, 0,
		&state.input.delta_x, &state.input.delta_y);
	if (player_at_edge) return 0;
	if (state.call_stack.call != &call_storage) return 1;

	// The engine pops the queued touch before resuming the production callback.
	state.call_stack.call = NULL;
	state.call_stack.curr_call = &call_storage;
	state.call_stack.state = 1;
	zoo_e_board_edge_touch(&state, source_x + dx, source_y + dy, 0,
		&state.input.delta_x, &state.input.delta_y);
	return 0;
}

int run_board_edge_compat_checks(void) {
	for (size_t i = 0; i < sizeof(board_edge_output); i++) ((unsigned char *) board_edge_output)[i] = 0;
	if (invoke_edge_touch(ZOO_E_EMPTY, false, 1, 30, 1, 0, -1, 30, 25) != 0) return 1;
	capture_board_edge_row(0);
	if (state.world.info.current_board != 1 || state.board.stats[0].x != 30
		|| state.board.stats[0].y != 25 || state.board.tiles[30][25].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].element != ZOO_E_EMPTY || state.input.delta_y != 0
		|| transition_count != 1 || board_enter_count != 1 || board_change_count != 1
		|| state.board.info.start_player_x != 30 || state.board.info.start_player_y != 25
		|| state.world.info.board_time_sec != 0) return 2;

	if (invoke_edge_touch(ZOO_E_EMPTY, false, 2, 30, 25, 0, 1, 30, 1) != 0) return 3;
	capture_board_edge_row(1);
	if (state.world.info.current_board != 2 || state.board.stats[0].x != 30
		|| state.board.stats[0].y != 1 || state.board.tiles[30][1].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].element != ZOO_E_EMPTY || state.input.delta_y != 0
		|| transition_count != 1 || board_enter_count != 1 || board_change_count != 1
		|| state.board.info.start_player_x != 30 || state.board.info.start_player_y != 1
		|| state.world.info.board_time_sec != 0) return 4;

	if (invoke_edge_touch(ZOO_E_EMPTY, false, 3, 1, 12, -1, 0, 60, 12) != 0) return 5;
	capture_board_edge_row(2);
	if (state.world.info.current_board != 3 || state.board.stats[0].x != 60
		|| state.board.stats[0].y != 12 || state.board.tiles[60][12].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].element != ZOO_E_EMPTY || state.input.delta_x != 0
		|| transition_count != 1 || board_enter_count != 1 || board_change_count != 1
		|| state.board.info.start_player_x != 60 || state.board.info.start_player_y != 12
		|| state.world.info.board_time_sec != 0) return 6;

	if (invoke_edge_touch(ZOO_E_EMPTY, false, 4, 60, 12, 1, 0, 1, 12) != 0) return 7;
	capture_board_edge_row(3);
	if (state.world.info.current_board != 4 || state.board.stats[0].x != 1
		|| state.board.stats[0].y != 12 || state.board.tiles[1][12].element != ZOO_E_PLAYER
		|| state.board.tiles[30][12].element != ZOO_E_EMPTY || state.input.delta_x != 0
		|| transition_count != 1 || board_enter_count != 1 || board_change_count != 1
		|| state.board.info.start_player_x != 1 || state.board.info.start_player_y != 12
		|| state.world.info.board_time_sec != 0) return 8;

	if (invoke_edge_touch(ZOO_E_SOLID, false, 1, 30, 1, 0, -1, 30, 25) != 0) return 9;
	capture_board_edge_row(4);
	if (state.world.info.current_board != 0 || state.board.stats[0].x != 30
		|| state.board.stats[0].y != 1 || state.input.delta_y != -1
		|| transition_count != 0 || board_enter_count != 0 || board_change_count != 2) return 10;

	if (invoke_edge_touch(ZOO_E_PLAYER, true, 1, 30, 1, 0, -1, 30, 25) != 0) return 11;
	capture_board_edge_row(5);
	if (state.world.info.current_board != 1 || state.board.stats[0].x != 30
		|| state.board.stats[0].y != 25 || state.input.delta_y != 0
		|| transition_count != 1 || board_enter_count != 1 || board_change_count != 1) return 12;
	return 0;
}
