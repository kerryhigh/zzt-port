#include <stddef.h>
#include <stdint.h>
#include <zoo.h>
#include <string.h>
#include "zzt_frame_scheduler.h"

static void test_tick(zoo_state *state, int16_t stat_id);

#include "../../third_party/libzoo/src/libzoo/zoo_game_io.c"
#include "../../third_party/libzoo/src/libzoo/zoo_game.c"

const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
    [1] = {.tick_func = test_tick}
};

static zoo_state state;
static zoo_video_driver video;
static int test_tick_count;
static uint8_t allocation[2048];
static size_t allocation_size;
static int sidebar_calls;
static uint16_t sidebar_flags;
static bool elapsed_ready;
static int16_t zero_speed_schedule_output[5];
static bool schedule_clock_enabled;
static int16_t game_schedule_output[117];
static int16_t frame_scheduler_output[11];
static int16_t frame_gameplay_output[24];

typedef struct {
    int16_t speed;
    int16_t start_hsecs;
    int16_t stall_after_poll;
    int16_t stall_extra_pits;
} game_schedule_case;

static const game_schedule_case game_schedule_cases[9] = {
    {0, 0, 0, 0},
    {0, 5980, 0, 0},
    {1, 0, 0, 0},
    {1, 5980, 0, 0},
    {6, 0, 0, 0},
    {6, 5980, 0, 0},
    {9, 0, 0, 0},
    {9, 5980, 0, 0},
    {6, 0, 2, 4}
};

static int16_t test_random(zoo_state *s, int16_t max) {
    (void) s;
    return max > 0 ? (int16_t) (max - 1) : 0;
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

size_t strnlen(const char *text, size_t limit) {
    size_t length = 0;
    while (length < limit && text[length] != '\0') length++;
    return length;
}

void *malloc(size_t size) {
    if (size > sizeof(allocation) || allocation_size != 0) return NULL;
    allocation_size = size;
    return allocation;
}

void free(void *ptr) {
    if (ptr == allocation) allocation_size = 0;
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

void zoo_stat_free(zoo_stat *stat) {
	if (stat != NULL && !stat->data_in_heap) free(stat->data);
}

void zoo_call_pop(zoo_call_stack *stack) { (void) stack; }
void zoo_sound_clear_queue(zoo_sound_state *s) { (void) s; }
bool zoo_has_hsecs_elapsed(zoo_state *s, int16_t *counter, int16_t hsecs) {
    if (schedule_clock_enabled) {
        int16_t total = (int16_t) (s->time_elapsed / 10);
        int16_t difference = (int16_t) ((total - *counter + 6000) % 6000);
        if (difference >= hsecs) {
            *counter = total;
            return true;
        }
        return false;
    }
    (void) s; (void) counter;
    return hsecs <= 0 || elapsed_ready;
}
void zoo_draw_player_surroundings(zoo_state *s, int16_t x, int16_t y, int16_t phase) {
    (void) s; (void) x; (void) y; (void) phase;
}
void zoo_sound_queue(zoo_sound_state *s, int16_t priority, const uint8_t *data, int16_t len) {
    (void) s; (void) priority; (void) data; (void) len;
}
void zoo_input_update(zoo_input_state *s) { (void) s; }
void zoo_input_clear(zoo_input_state *s) { (void) s; }

static void test_tick(zoo_state *s, int16_t stat_id) {
    (void) s; (void) stat_id; test_tick_count++;
}

static void test_draw_sidebar(zoo_state *s, uint16_t flags) {
    (void) s;
    sidebar_calls++;
    sidebar_flags = flags;
}

static void test_video_write(zoo_video_driver *drv, int16_t x, int16_t y, uint8_t col, uint8_t chr) {
    (void) drv; (void) x; (void) y; (void) col; (void) chr;
}

static bool text_equals(const char *actual, const char *expected, size_t length) {
    size_t i;
    for (i = 0; i < length; i++) {
        if (actual[i] != expected[i]) return false;
    }
    return actual[length] == '\0';
}

static void reset_state(void) {
    uint8_t *bytes = (uint8_t *) &state;
    size_t i;
    for (i = 0; i < sizeof(state); i++) bytes[i] = 0;
    for (i = 0; i < sizeof(video); i++) ((uint8_t *) &video)[i] = 0;
    test_tick_count = 0;
    sidebar_calls = 0;
    sidebar_flags = 0;
    elapsed_ready = false;
    allocation_size = 0;
    state.game_state = GS_PLAY;
    state.func_random = test_random;
    state.func_draw_sidebar = test_draw_sidebar;
    video.func_write = test_video_write;
    state.d_video = &video;
    state.current_tick = 1;
    state.tick_duration = 8;
    state.board.stat_count = 1;
    state.board.stats[1].cycle = 1;
    state.board.stats[1].x = 1;
    state.board.stats[1].y = 1;
    state.board.tiles[1][1].element = 1;
}

int run_exit_tick_checks(void) {
    zoo_tick_retval result;

    reset_state();
    state.game_play_exit_requested = true;
    state.current_stat_tick = 1;
    result = zoo_tick(&state);
    if (result != RETURN_NEXT_CYCLE || test_tick_count != 0 || state.current_stat_tick != 1) return 1;
    result = zoo_tick(&state);
    if (result != RETURN_NEXT_CYCLE || test_tick_count != 0 || state.current_stat_tick != 1) return 2;

    reset_state();
    state.current_stat_tick = 1;
    result = zoo_tick(&state);
    if (result != RETURN_NEXT_CYCLE || test_tick_count != 1 || state.current_stat_tick != 2) return 3;

    reset_state();
    state.game_play_exit_requested = true;
    state.game_tick_state = 2;
    state.current_stat_tick = 1;
    state.board.stat_count = 2;
    result = zoo_tick(&state);
    if (result != RETURN_NEXT_CYCLE || test_tick_count != 0 || state.current_stat_tick != 2) return 4;

    // Pascal's zero-duration elapsed check immediately begins the next
    // simulation cycle; it does not wait for a host display frame.
    reset_state();
    state.current_stat_tick = 2;
    state.tick_duration = 0;
    result = zoo_tick(&state);
    if (result != RETURN_IMMEDIATE || state.current_tick != 2
            || state.current_stat_tick != 0) return 5;

    // A positive duration still yields to the next PIT cycle when elapsed.
    reset_state();
    state.current_stat_tick = 2;
    state.tick_duration = 8;
    elapsed_ready = true;
    result = zoo_tick(&state);
    if (result != RETURN_NEXT_CYCLE || state.current_tick != 2
            || state.current_stat_tick != 0) return 6;

    return 0;
}

int run_zero_speed_schedule_checks(void) {
    int i;
    zoo_tick_retval result;

    reset_state();
    state.current_tick = 100;
    state.current_stat_tick = 1;
    state.board.stat_count = 0;
    state.tick_duration = 0;
    for (i = 0; i < 5; i++) {
        result = zoo_tick(&state);
        if (result != RETURN_IMMEDIATE) return i + 1;
    }

    zero_speed_schedule_output[0] = 5;
    zero_speed_schedule_output[1] = (int16_t) (state.current_tick - 100);
    zero_speed_schedule_output[2] = state.current_stat_tick;
    zero_speed_schedule_output[3] = (int16_t) state.time_elapsed;
    zero_speed_schedule_output[4] = state.tick_counter;
    if (test_tick_count != 0 || state.game_play_exit_requested) return 6;
    return 0;
}

int run_game_schedule_checks(void) {
    int case_index;
    schedule_clock_enabled = true;
    for (case_index = 0; case_index < 9; case_index++) {
        const game_schedule_case *test_case = &game_schedule_cases[case_index];
        int16_t *row = &game_schedule_output[case_index * 13];
        double elapsed_ms = (double) test_case->start_hsecs * 10.0;
        int cycles = 0;
        int polls;
        reset_state();
        state.game_state = GS_PLAY;
        state.game_paused = false;
        state.current_tick = 100;
        state.current_stat_tick = 1;
        state.board.stat_count = 0;
        state.tick_speed = test_case->speed;
        state.tick_duration = (int16_t) (test_case->speed * 2);
        state.tick_counter = test_case->start_hsecs;
        state.time_elapsed = elapsed_ms;
        elapsed_ready = false;

        for (polls = 0; polls < 256 && cycles < 5; polls++) {
            int16_t previous_tick = state.current_tick;
            zoo_tick_retval result = zoo_tick(&state);
            int advance_pits = 1;
            if ((test_case->speed == 0 && result != RETURN_IMMEDIATE)
                    || (test_case->speed > 0 && result != RETURN_NEXT_CYCLE)) {
                return 1 + case_index;
            }
            if (state.current_tick != previous_tick) {
                int16_t event_hsecs = (int16_t) ((int16_t) (state.time_elapsed / 10) % 6000);
                row[8 + cycles] = event_hsecs;
                cycles++;
            }
            if (test_case->stall_after_poll == polls + 1) {
                advance_pits += test_case->stall_extra_pits;
            }
            elapsed_ms += advance_pits * ZOO_PIT_TICK_MS;
            while (elapsed_ms >= 60000.0) elapsed_ms -= 60000.0;
            state.time_elapsed = elapsed_ms;
        }
        if (cycles != 5) return 6 + case_index;
        row[0] = test_case->speed;
        row[1] = test_case->start_hsecs;
        row[2] = test_case->stall_after_poll;
        row[3] = test_case->stall_extra_pits;
        row[4] = (int16_t) cycles;
        row[5] = (int16_t) (state.current_tick - 100);
        row[6] = state.current_stat_tick;
        row[7] = state.tick_counter;
        if (row[5] != 5 || row[6] != 0 || row[7] != row[12]) return 11 + case_index;
    }
    schedule_clock_enabled = false;
    return 0;
}

uint32_t game_schedule_output_ptr(void) {
    return (uint32_t) (uintptr_t) game_schedule_output;
}

int run_frame_scheduler_checks(void) {
    zzt_frame_scheduler scheduler;
    double now_ms;
    int pit_ticks;
    int total_pits;
    int i;
    int64_t timestamp;
    int64_t base = 10000000000LL;
    double previous_deadline;

    zzt_frame_scheduler_reset(&scheduler);
    total_pits = zzt_frame_scheduler_begin_frame(&scheduler, base, &now_ms);
    for (i = 1; i <= 120; i++) {
        timestamp = base + (int64_t) i * 16666667LL;
        total_pits += zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    }
    if (total_pits != 36 || scheduler.pit_remainder_ms < 22.7
            || scheduler.pit_remainder_ms > 22.8) return 1;
    frame_scheduler_output[0] = (int16_t) total_pits;

    zzt_frame_scheduler_reset(&scheduler);
    timestamp = base;
    if (zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms) != 0
            || !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 2;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);
    for (i = 1; i <= 4; i++) {
        timestamp = base + (int64_t) i * 16666667LL;
        pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
        if (i < 4 && pit_ticks != 0) return 3;
        if (i == 4 && (pit_ticks != 1 || !zzt_frame_scheduler_game_due(&scheduler, now_ms))) return 4;
    }
    frame_scheduler_output[1] = 1;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);

    timestamp = base + 66666668LL + 300000000LL;
    pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (pit_ticks != 5 || !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 5;
    frame_scheduler_output[2] = (int16_t) pit_ticks;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);

    timestamp += 16666667LL;
    pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (pit_ticks != 0 || !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 6;
    frame_scheduler_output[3] = (int16_t) pit_ticks;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);

    timestamp += 1200000001LL;
    pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (pit_ticks != 0 || scheduler.previous_frame_nanos != timestamp
            || !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 7;
    frame_scheduler_output[4] = (int16_t) pit_ticks;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);

    total_pits = 0;
    for (i = 1; i <= 4; i++) {
        timestamp += 16666667LL;
        total_pits += zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
        if (i < 4 && zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 8;
        if (i == 4 && !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 9;
    }
    if (total_pits != 2) return 10;
    frame_scheduler_output[5] = (int16_t) total_pits;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_CYCLE);

    timestamp -= 100000000LL;
    pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (pit_ticks != 0 || scheduler.previous_frame_nanos != timestamp
            || !zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 11;
    frame_scheduler_output[6] = (int16_t) pit_ticks;

    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, RETURN_NEXT_FRAME);
    timestamp += 15900000LL;
    (void) zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 12;
    frame_scheduler_output[7] = 0;
    timestamp += 16100000LL;
    (void) zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
    if (!zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 13;
    frame_scheduler_output[8] = 1;

    zzt_frame_scheduler_modal_prompt(&scheduler, now_ms);
    if (!zzt_frame_scheduler_game_due(&scheduler, now_ms)) return 14;
    frame_scheduler_output[9] = 1;
    previous_deadline = scheduler.next_game_ms;
    zzt_frame_scheduler_tick_finished(&scheduler, now_ms, ERROR);
    if (scheduler.next_game_ms != previous_deadline) return 15;
    frame_scheduler_output[10] = 1;
    return 0;
}

uint32_t frame_scheduler_output_ptr(void) {
    return (uint32_t) (uintptr_t) frame_scheduler_output;
}

static int run_frame_gameplay_case(int stalled, int16_t *row) {
    zzt_frame_scheduler scheduler;
    int64_t timestamp = 10000000000LL;
    double now_ms;
    int frame;
    int total_pits = 0;
    int game_calls = 0;
    int cycles = 0;
    bool stall_inserted = false;
    reset_state();
    schedule_clock_enabled = true;
    state.game_state = GS_PLAY;
    state.game_paused = false;
    state.current_tick = 100;
    state.current_stat_tick = 1;
    state.board.stat_count = 0;
    state.tick_speed = 6;
    state.tick_duration = 12;
    state.tick_counter = 0;
    state.time_elapsed = 0;
    zzt_frame_scheduler_reset(&scheduler);

    for (frame = 0; frame < 1000 && cycles < 5; frame++) {
        int pit_ticks;
        if (frame > 0) {
            if (stalled && !stall_inserted && game_calls >= 2) {
                timestamp += 300000000LL;
                stall_inserted = true;
            } else {
                timestamp += 16666667LL;
            }
        }
        pit_ticks = zzt_frame_scheduler_begin_frame(&scheduler, timestamp, &now_ms);
        total_pits += pit_ticks;
        while (pit_ticks-- > 0) zoo_tick_advance_pit(&state);
        if (!zzt_frame_scheduler_game_due(&scheduler, now_ms)) continue;

        for (;;) {
            int16_t previous_tick = state.current_tick;
            zoo_tick_retval result = zoo_tick(&state);
            game_calls++;
            if (state.current_tick != previous_tick && cycles < 5) {
                row[7 + cycles] = (int16_t) ((int16_t) (state.time_elapsed / 10) % 6000);
                cycles++;
            }
            if (result == RETURN_IMMEDIATE) continue;
            if (result != RETURN_NEXT_CYCLE && result != RETURN_NEXT_FRAME && result != ERROR) {
                schedule_clock_enabled = false;
                return 1;
            }
            zzt_frame_scheduler_tick_finished(&scheduler, now_ms, result);
            break;
        }
    }

    if (cycles != 5 || (stalled && !stall_inserted)) {
        schedule_clock_enabled = false;
        return 2;
    }
    row[0] = (int16_t) stalled;
    row[1] = (int16_t) total_pits;
    row[2] = (int16_t) game_calls;
    row[3] = (int16_t) cycles;
    row[4] = (int16_t) (state.current_tick - 100);
    row[5] = state.current_stat_tick;
    row[6] = state.tick_counter;
    schedule_clock_enabled = false;
    return 0;
}

int run_frame_gameplay_checks(void) {
    int result;
    result = run_frame_gameplay_case(0, &frame_gameplay_output[0]);
    if (result != 0) return result;
    result = run_frame_gameplay_case(1, &frame_gameplay_output[12]);
    if (result != 0) return result + 10;
    return 0;
}

uint32_t frame_gameplay_output_ptr(void) {
    return (uint32_t) (uintptr_t) frame_gameplay_output;
}

uint32_t zero_speed_schedule_output_ptr(void) {
    return (uint32_t) (uintptr_t) zero_speed_schedule_output;
}

int run_game_start_enter_checks(void) {
    reset_state();
    state.time_elapsed = 3210;
    state.tick_counter = 321;
    state.world.info.board_time_sec = 37;
    state.board.info.start_player_x = 7;
    state.board.info.start_player_y = 8;
    state.board.stats[0].x = 10;
    state.board.stats[0].y = 11;
    state.board.stats[1].x = 1;
    state.board.stats[1].y = 1;
    zoo_game_start(&state, GS_TITLE);
    if (state.world.info.board_time_sec != 37
            || state.board.info.start_player_x != 7 || state.board.info.start_player_y != 8
            || state.game_paused || state.time_elapsed != 3210 || state.tick_counter != 321) return 1;

    reset_state();
    state.time_elapsed = 6540;
    state.tick_counter = 654;
    state.world.info.board_time_sec = 53;
    state.board.info.start_player_x = 3;
    state.board.info.start_player_y = 4;
    state.board.stats[0].x = 12;
    state.board.stats[0].y = 19;
    state.board.stats[1].x = 1;
    state.board.stats[1].y = 1;
    zoo_game_start(&state, GS_PLAY);
    if (state.world.info.board_time_sec != 53
            || state.board.info.start_player_x != 3 || state.board.info.start_player_y != 4
            || !state.game_paused || state.time_elapsed != 6540 || state.tick_counter != 654) return 2;

    // BoardEnter records the selected spawn, resets the board timer, and only
    // creates the original one-time dark-room hint on the first dark entry.
    reset_state();
    state.board.stat_count = 0;
    state.world.info.board_time_sec = 37;
    state.board.info.start_player_x = 7;
    state.board.info.start_player_y = 8;
    state.board.stats[0].x = 12;
    state.board.stats[0].y = 19;
    state.msg_flags.hint_torch = true;
    zoo_board_enter(&state);
    if (state.world.info.board_time_sec != 0
            || state.board.info.start_player_x != 12 || state.board.info.start_player_y != 19
            || !state.msg_flags.hint_torch || state.board.stat_count != 0
            || sidebar_calls != 1 || sidebar_flags != ZOO_SIDEBAR_UPDATE_ALL) return 3;

    reset_state();
    state.board.stat_count = 0;
    state.world.info.board_time_sec = 53;
    state.board.info.start_player_x = 3;
    state.board.info.start_player_y = 4;
    state.board.stats[0].x = 13;
    state.board.stats[0].y = 17;
    state.board.info.is_dark = true;
    state.board.tiles[0][0].element = ZOO_E_BOARD_EDGE;
    state.msg_flags.hint_torch = true;
    zoo_board_enter(&state);
    if (state.world.info.board_time_sec != 0
            || state.board.info.start_player_x != 13 || state.board.info.start_player_y != 17
            || state.msg_flags.hint_torch || state.board.stat_count != 1
            || state.board.stats[1].x != 0 || state.board.stats[1].y != 0
            || state.board.stats[1].cycle != 1 || state.board.stats[1].p2 != 22
            || state.board.tiles[0][0].element != ZOO_E_MESSAGE_TIMER
            || !text_equals(state.board.info.message,
                "Room is dark - you need to light a torch!", 41)
            || sidebar_calls != 1 || sidebar_flags != ZOO_SIDEBAR_UPDATE_ALL) return 4;

    state.world.info.board_time_sec = 24;
    state.board.stats[0].x = 20;
    state.board.stats[0].y = 12;
    zoo_board_enter(&state);
    if (state.world.info.board_time_sec != 0
            || state.board.info.start_player_x != 20 || state.board.info.start_player_y != 12
            || state.msg_flags.hint_torch || state.board.stat_count != 1
            || state.board.stats[1].p2 != 22
            || !text_equals(state.board.info.message,
                "Room is dark - you need to light a torch!", 41)
            || sidebar_calls != 2 || sidebar_flags != ZOO_SIDEBAR_UPDATE_ALL) return 5;
    return 0;
}
