#include <stdint.h>
#include <stddef.h>
#include "../../third_party/libzoo/src/libzoo/zoo_game_io.c"
#include "../../third_party/libzoo/src/libzoo/zoo_game.c"
#include "../../third_party/libzoo/src/libzoo/zoo_io.c"
#include "../../third_party/libzoo/src/libzoo/zoo_oop.c"
#include "../../third_party/libzoo/src/libzoo/zoo_editor_brd.c"
#define FIXTURE_CAP 131072
#define HEAP_CAP 2097152
static uint8_t input_data[FIXTURE_CAP];
static uint8_t output_data[FIXTURE_CAP];
static uint8_t heap_data[HEAP_CAP];
static size_t heap_used;
static size_t output_size;
static zoo_world world;
static zoo_state state;
static zoo_board test_board;
const zoo_element_def zoo_element_defs[ZOO_MAX_ELEMENT + 1] = {
    [ZOO_E_PLAYER] = { .color = 0x1F }
};
void *memcpy(void *dest, const void *src, size_t count) { unsigned char *d = dest; const unsigned char *s = src; size_t i; for (i = 0; i < count; ++i) d[i] = s[i]; return dest; }
void *memset(void *dest, int value, size_t count) { unsigned char *d = dest; size_t i; for (i = 0; i < count; ++i) d[i] = (unsigned char)value; return dest; }
int memcmp(const void *left, const void *right, size_t count) { const unsigned char *a = left, *b = right; size_t i; for (i = 0; i < count; ++i) if (a[i] != b[i]) return (int)a[i] - (int)b[i]; return 0; }
size_t strnlen(const char *str, size_t maxlen) { size_t n = 0; while (n < maxlen && str[n] != 0) ++n; return n; }
int strncmp(const char *left, const char *right, size_t count) { size_t i; for (i = 0; i < count; ++i) { unsigned char a = (unsigned char)left[i], b = (unsigned char)right[i]; if (a != b || a == 0) return (int)a - (int)b; } return 0; }
char *strncpy(char *dest, const char *src, size_t count) { size_t i = 0; while (i < count && src[i] != 0) { dest[i] = src[i]; ++i; } while (i < count) dest[i++] = 0; return dest; }
void *malloc(size_t size) { size_t at = (heap_used + 7u) & ~(size_t)7u; if (size > HEAP_CAP - at) return NULL; heap_used = at + size; return heap_data + at; }
void free(void *ptr) { (void)ptr; }
void *realloc(void *ptr, size_t size) { void *next = malloc(size); if (next != NULL && ptr != NULL) memcpy(next, ptr, size); return next; }
uint32_t input_ptr(void) { return (uint32_t)(uintptr_t)input_data; }
uint32_t output_ptr(void) { return (uint32_t)(uintptr_t)output_data; }
uint32_t output_len(void) { return (uint32_t)output_size; }
int run_roundtrip(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0; output_size = 0;
    memset(&world, 0, sizeof(world)); memset(&state, 0, sizeof(state)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    state.world = world;
    ret = zoo_board_open(&state, state.world.info.current_board);
    if (ret != 0) return ret;
    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    world = state.world;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &world);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}

int run_signed_serialization_codec(void) {
    zoo_io_handle out_handle;
    zoo_io_handle in_handle;
    uint32_t bits;

    memset(output_data, 0, sizeof(output_data));
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    for (bits = 0; bits <= 0xFFFFu; ++bits) {
        uint16_t raw = (uint16_t) bits;
        int16_t expected = zoo_io_i16_from_u16(raw);
        size_t at = (size_t) bits * 2;
        zoo_io_write_short(&out_handle, expected);
        if (output_data[at] != (uint8_t) raw
                || output_data[at + 1] != (uint8_t) (raw >> 8)
                || zoo_editor_brd_i16(output_data + at) != expected) return 1;
    }
    if (out_handle.func_tell(&out_handle) != FIXTURE_CAP) return 2;

    in_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_READ);
    for (bits = 0; bits <= 0xFFFFu; ++bits) {
        if (zoo_io_read_short(&in_handle) != zoo_io_i16_from_u16((uint16_t) bits)) return 3;
    }
    if (in_handle.func_tell(&in_handle) != FIXTURE_CAP) return 5;
    for (bits = 0; bits <= 0xFFu; ++bits) {
        uint8_t raw = (uint8_t) bits;
        int8_t expected = (raw <= 0x7Fu)
                ? (int8_t) raw
                : (int8_t) ((int16_t) raw - 0x100);
        if (zoo_io_i8_from_u8(raw) != expected) return 4;
    }
    return 0;
}

int run_pascal_boardclose_worldsave(uint32_t input_len) {
    zoo_io_handle in_handle;
    zoo_io_handle out_handle;
    int ret;
    int x, y;
    int16_t board_id;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0; output_size = 0;
    memset(&state, 0, sizeof(state));
    memset(output_data, 0, sizeof(output_data));

    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &state.world, false);
    if (ret != 0) return ret;
    board_id = state.world.info.current_board;
    ret = zoo_board_open(&state, board_id);
    if (ret != 0) return ret;

    /* Match BOARDENC.PAS: overwrite the active board's one-based playfield. */
    for (x = 1; x <= ZOO_BOARD_WIDTH; ++x) {
        for (y = 1; y <= ZOO_BOARD_HEIGHT; ++y) {
            state.board.tiles[x][y].element = ZOO_E_EMPTY;
            state.board.tiles[x][y].color = 0x70;
        }
    }

    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &state.world);
    if (ret != 0) return ret;
    output_size = (size_t) out_handle.func_tell(&out_handle);
    return 0;
}

int run_game_save_roundtrip(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0; output_size = 0;
    memset(&world, 0, sizeof(world)); memset(&state, 0, sizeof(state)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    state.world = world;
    ret = zoo_board_open(&state, state.world.info.current_board);
    if (ret != 0) return ret;
    /* Pascal sets IsSave when play starts; this exercises the production save-close/open path. */
    state.world.info.is_save = true;
    state.world.info.io_is_save_byte = 1;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_world_save(&state, &out_handle);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}

static int signed_world_fields_match(const zoo_world_info *info, int16_t expected) {
    return info->ammo == expected
        && info->gems == expected
        && info->health == expected
        && info->torches == expected
        && info->torch_ticks == expected
        && info->energizer_ticks == expected
        && info->score == expected
        && info->board_time_sec == expected
        && info->board_time_hsec == expected;
}

int run_world_signed_field_read(uint32_t input_len, int32_t expected_raw) {
    zoo_io_handle in_handle;
    int result;
    if (input_len > FIXTURE_CAP || expected_raw < -32768 || expected_raw > 32767) return -90;
    heap_used = 0;
    memset(&world, 0, sizeof(world));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    result = zoo_io_world_read(&in_handle, &world, false);
    if (result != 0) return result;
    return signed_world_fields_match(&world.info, (int16_t) expected_raw) ? 0 : 1;
}

int run_game_signed_world_save(uint32_t input_len, int32_t expected_raw) {
    int result;
    int16_t expected;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP || expected_raw < -32768 || expected_raw > 32767) return -90;
    expected = (int16_t) expected_raw;
    heap_used = 0;
    output_size = 0;
    memset(&world, 0, sizeof(world));
    memset(&state, 0, sizeof(state));
    memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    result = zoo_io_world_read(&in_handle, &world, false);
    if (result != 0) return result;
    state.world = world;
    result = zoo_board_open(&state, state.world.info.current_board);
    if (result != 0) return result;
    state.world.info.ammo = expected;
    state.world.info.gems = expected;
    state.world.info.health = expected;
    state.world.info.torches = expected;
    state.world.info.torch_ticks = expected;
    state.world.info.energizer_ticks = expected;
    state.world.info.score = expected;
    state.world.info.board_time_sec = expected;
    state.world.info.board_time_hsec = expected;
    state.world.info.is_save = true;
    state.world.info.io_is_save_byte = 1;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    result = zoo_world_save(&state, &out_handle);
    if (result != 0) return result;
    if (!signed_world_fields_match(&state.world.info, expected)) return 1;
    output_size = (size_t) out_handle.func_tell(&out_handle);
    return 0;
}

int run_world_read(uint32_t input_len) {
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(&world, 0, sizeof(world));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    return zoo_io_world_read(&in_handle, &world, false);
}

int run_world_read_preserves_tail(uint32_t input_len, uint32_t expected_board_len) {
    int ret;
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP || expected_board_len == 0 || expected_board_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(heap_data, 0xA5, sizeof(heap_data));
    memset(&world, 0, sizeof(world));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    if (world.board_len[0] != (int16_t) expected_board_len || world.board_data[0] == NULL) return -91;
    if (((uint8_t *) world.board_data[0])[expected_board_len - 1] != 0xA5) return -92;
    return 0;
}

int run_world_read_title_only(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(&world, 0, sizeof(world));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, true);
    if (ret != 0) return ret;
    if (world.board_count != 0 || world.info.current_board != 0
            || !world.info.is_save || world.info.io_is_save_byte != 1) return -91;
    if (world.board_len[0] <= 0 || world.board_data[0] == NULL) return -92;
    return 0;
}

int run_world_load_missing_trailing(uint32_t input_len) {
    int ret;
    int16_t i;
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(&state, 0, sizeof(state));
    zoo_world_create(&state);
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_world_load(&state, &in_handle, false);
    if (ret != 0) return ret;
    if (state.world.board_count != 15 || state.world.info.current_board != 0
            || state.world.info.is_save || state.world.board_len[0] <= 0) return -91;
    for (i = 1; i <= state.world.board_count; i++) {
        if (state.world.board_len[i] != 0 || state.world.board_data[i] != NULL) return -92;
    }
    return 0;
}

int run_world_load_partial_length(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(&state, 0, sizeof(state));
    zoo_world_create(&state);
    state.world.board_count = 1;
    state.world.board_len[1] = 0x0100;
    state.world.board_data[1] = malloc(0x0100);
    if (state.world.board_data[1] == NULL) return ZOO_ERROR_NOMEM;
    memset(state.world.board_data[1], 0x5A, 0x0100);
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_world_load(&state, &in_handle, false);
    if (ret != 0) return ret;
    if (state.world.board_count != 15 || state.world.info.current_board != 0
            || state.world.info.is_save || state.world.board_len[1] != 0x0110
            || state.world.board_data[1] == NULL) return -91;
    return 0;
}

int run_world_load_unsigned_board_length(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    memset(&state, 0, sizeof(state));
    zoo_world_create(&state);
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_world_load(&state, &in_handle, false);
    if (ret != 0) return ret;
    if (state.world.board_count != 0 || state.world.info.current_board != 0
            || state.world.info.is_save || state.world.board_len[0] != -1
            || state.world.board_data[0] == NULL) return -91;
    return 0;
}

int run_flag_save_roundtrip(uint32_t input_len, uint32_t action) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP || action > 2) return -90;
    heap_used = 0; output_size = 0;
    memset(&world, 0, sizeof(world)); memset(&state, 0, sizeof(state)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    state.world = world;
    ret = zoo_board_open(&state, state.world.info.current_board);
    if (ret != 0) return ret;
    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    if (action == 0) zoo_flag_set(&state, "A");
    else if (action == 1) zoo_flag_clear(&state, "DUP");
    else {
        zoo_flag_set(&state, "LONG");
        zoo_flag_clear(&state, "LONG");
        zoo_flag_set(&state, "A");
    }
    world = state.world;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &world);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}

int run_repeated_pstring_assignment_roundtrip(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0; output_size = 0;
    memset(&world, 0, sizeof(world)); memset(&state, 0, sizeof(state)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    state.world = world;
    ret = zoo_board_open(&state, state.world.info.current_board);
    if (ret != 0) return ret;

    zoo_io_pstring_assign_pascal(state.world.info.name, sizeof(state.world.info.name) - 1,
        state.world.info.io_name_data, &state.world.info.io_name_length,
        &state.world.info.io_name_valid, "LONG", 4);
    zoo_io_pstring_assign_pascal(state.world.info.name, sizeof(state.world.info.name) - 1,
        state.world.info.io_name_data, &state.world.info.io_name_length,
        &state.world.info.io_name_valid, "X", 1);
    zoo_io_pstring_assign_pascal(state.world.info.name, sizeof(state.world.info.name) - 1,
        state.world.info.io_name_data, &state.world.info.io_name_length,
        &state.world.info.io_name_valid, "AB", 2);

    zoo_io_pstring_assign_pascal(state.board.name, sizeof(state.board.name) - 1,
        state.board.io_name_data, &state.board.io_name_length,
        &state.board.io_name_valid, "LONG", 4);
    zoo_io_pstring_assign_pascal(state.board.name, sizeof(state.board.name) - 1,
        state.board.io_name_data, &state.board.io_name_length,
        &state.board.io_name_valid, "X", 1);
    zoo_io_pstring_assign_pascal(state.board.name, sizeof(state.board.name) - 1,
        state.board.io_name_data, &state.board.io_name_length,
        &state.board.io_name_valid, "AB", 2);

    zoo_io_pstring_assign_pascal(state.board.info.message, ZOO_LEN_MESSAGE,
        state.board.info.io_message_data, &state.board.info.io_message_length,
        &state.board.info.io_message_valid, "LONG", 4);
    zoo_io_pstring_assign_pascal(state.board.info.message, ZOO_LEN_MESSAGE,
        state.board.info.io_message_data, &state.board.info.io_message_length,
        &state.board.info.io_message_valid, "X", 1);
    zoo_io_pstring_assign_pascal(state.board.info.message, ZOO_LEN_MESSAGE,
        state.board.info.io_message_data, &state.board.info.io_message_length,
        &state.board.info.io_message_valid, "", 0);

    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    world = state.world;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &world);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}

int run_pstring_matches_pascal_checks(void) {
    char text[51] = {'A', 0};
    char embedded_value[3] = {'A', 0, 'B'};
    char shortened_value[1] = {'A'};
    uint8_t raw_data[50] = {'A', 0, 'B'};
    uint8_t full_value[50];

    if (!zoo_io_pstring_matches_pascal(text, 50, raw_data, 3, true,
            embedded_value, sizeof(embedded_value))) return 1;
    if (zoo_io_pstring_matches_pascal(text, 50, raw_data, 3, true,
            shortened_value, sizeof(shortened_value))) return 2;

    for (size_t i = 0; i < sizeof(full_value); i++) full_value[i] = (uint8_t) (i * 7 + 3);
    memcpy(raw_data, full_value, sizeof(raw_data));
    if (!zoo_io_pstring_matches_pascal(text, 50, raw_data, 255, true,
            (const char *) full_value, sizeof(full_value))) return 3;

    memset(text, 0, sizeof(text));
    memcpy(text, "VISIBLE", 7);
    if (!zoo_io_pstring_matches_pascal(text, 50, raw_data, 0, false, "VISIBLE", 7)) return 4;
    if (zoo_io_pstring_matches_pascal(text, 50, raw_data, 0, false, "VISIBLE!", 8)) return 5;
    return 0;
}

int run_world_reset_string_roundtrip(uint32_t input_len) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (input_len > FIXTURE_CAP) return -90;
    heap_used = 0; output_size = 0;
    memset(&world, 0, sizeof(world)); memset(&state, 0, sizeof(state)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data, input_len, MODE_READ);
    ret = zoo_io_world_read(&in_handle, &world, false);
    if (ret != 0) return ret;
    state.world = world;
    ret = zoo_board_open(&state, state.world.info.current_board);
    if (ret != 0) return ret;
    ret = zoo_world_close(&state);
    if (ret != 0) return ret;
    zoo_world_create(&state);
    if (state.world.board_count != 0 || state.world.info.current_board != 0
            || state.world.info.ammo != 0 || state.world.info.gems != 0
            || state.world.info.health != 100 || state.world.info.torches != 0
            || state.world.info.torch_ticks != 0 || state.world.info.energizer_ticks != 0
            || state.world.info.score != 0 || state.world.info.board_time_sec != 0
            || state.world.info.board_time_hsec != 0 || state.world.info.is_save) return -91;
    if (state.world.info.name[0] != 0 || !state.world.info.io_name_valid
            || state.world.info.io_name_length != 0) return -92;
    for (int i = 0; i < ZOO_MAX_FLAG; i++) {
        if (state.world.info.flags[i][0] != 0 || !state.world.info.io_flag_valid[i]
                || state.world.info.io_flag_lengths[i] != 0) return -93;
    }
    for (int i = 0; i < 7; i++) {
        if (state.world.info.keys[i] || state.world.info.io_key_bytes[i] != 0) return -94;
    }
    if (strncmp(state.board.name, "Title screen", sizeof(state.board.name)) != 0
            || state.board.stat_count != 0
            || state.board.stats[0].x != ZOO_BOARD_WIDTH / 2
            || state.board.stats[0].y != ZOO_BOARD_HEIGHT / 2
            || state.board.stats[0].cycle != 1
            || state.board.stats[0].data != NULL
            || state.board.stats[0].data_len != 0
            || state.board.tiles[ZOO_BOARD_WIDTH / 2][ZOO_BOARD_HEIGHT / 2].element != ZOO_E_PLAYER
            || state.board.tiles[ZOO_BOARD_WIDTH / 2][ZOO_BOARD_HEIGHT / 2].color
                    != zoo_element_defs[ZOO_E_PLAYER].color) return -95;
    if (state.board.info.max_shots != 255 || state.board.info.is_dark
            || state.board.info.reenter_when_zapped || state.board.info.time_limit_sec != 0
            || state.board.info.neighbor_boards[0] != 0 || state.board.info.neighbor_boards[1] != 0
            || state.board.info.neighbor_boards[2] != 0 || state.board.info.neighbor_boards[3] != 0) return -96;
    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &state.world);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}


int run_board_roundtrip(uint32_t offset, uint32_t board_len) {
    int ret;
    zoo_io_handle in_handle, out_handle;
    if (offset > FIXTURE_CAP || board_len > FIXTURE_CAP - offset) return -90;
    heap_used = 0; output_size = 0;
    memset(&test_board, 0, sizeof(test_board)); memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(input_data + offset, board_len, MODE_READ);
    ret = zoo_io_board_read(&in_handle, &test_board);
    if (ret != 0) return ret;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_board_write(&out_handle, &test_board);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    return 0;
}int run_zero_count_rle(uint32_t offset, uint32_t board_len) {
    zoo_io_handle in_handle;
    int ret;
    if (offset > FIXTURE_CAP || board_len > FIXTURE_CAP - offset) return -90;
    heap_used = 0;
    memset(&test_board, 0, sizeof(test_board));
    in_handle = zoo_io_open_file_mem(input_data + offset, board_len, MODE_READ);
    ret = zoo_io_board_read(&in_handle, &test_board);
    if (ret != 0) return ret;
    if (test_board.tiles[1][1].element != 7 || test_board.tiles[1][1].color != 8) return 1;
    if (test_board.tiles[60][1].element != 7 || test_board.tiles[1][2].element != 7) return 2;
    if (test_board.tiles[16][5].element != 7 || test_board.tiles[16][5].color != 8) return 3;
    if (test_board.tiles[17][5].element != 1 || test_board.tiles[17][5].color != 2) return 4;
    if (test_board.tiles[60][25].element != 1 || test_board.tiles[60][25].color != 2) return 5;
    return 0;
}int run_final_rle_overrun(uint32_t offset, uint32_t board_len) {
    zoo_io_handle in_handle;
    int ret;
    if (offset > FIXTURE_CAP || board_len > FIXTURE_CAP - offset) return -90;
    heap_used = 0;
    memset(&test_board, 0, sizeof(test_board));
    in_handle = zoo_io_open_file_mem(input_data + offset, board_len, MODE_READ);
    ret = zoo_io_board_read(&in_handle, &test_board);
    if (ret != 0) return ret;
    if (test_board.tiles[15][22].element != 5 || test_board.tiles[15][22].color != 15) return 1;
    if (test_board.tiles[16][22].element != 6 || test_board.tiles[16][22].color != 16) return 2;
    if (test_board.tiles[60][25].element != 6 || test_board.tiles[60][25].color != 16) return 3;
    if (test_board.info.max_shots != 77) return 4;
    return 0;
}

int run_board_rle_count(uint32_t count) {
    static uint8_t fixture[256];
    zoo_io_handle in_handle;
    uint32_t effective_count, remaining;
    size_t p;
    int ix, iy, ret;
    if (count > 255) return -90;

    memset(fixture, 0, sizeof(fixture));
    p = 51;
    fixture[p++] = (uint8_t)count;
    fixture[p++] = 7;
    fixture[p++] = 8;
    effective_count = count == 0 ? 256 : count;
    remaining = 1500u - effective_count;
    while (remaining > 0) {
        uint8_t run = remaining > 255 ? 255 : (uint8_t)remaining;
        fixture[p++] = run;
        fixture[p++] = 1;
        fixture[p++] = 2;
        remaining -= run;
    }
    fixture[p] = 77;
    p += 86;
    fixture[p++] = 0;
    fixture[p++] = 0;
    p += 33;
    if (p > sizeof(fixture)) return -91;

    heap_used = 0;
    output_size = 0;
    memset(&test_board, 0, sizeof(test_board));
    memset(output_data, 0, sizeof(output_data));
    in_handle = zoo_io_open_file_mem(fixture, p, MODE_READ);
    ret = zoo_io_board_read(&in_handle, &test_board);
    if (ret != 0) return ret;

    for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
        for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
            output_data[output_size++] = test_board.tiles[ix][iy].element;
            output_data[output_size++] = test_board.tiles[ix][iy].color;
        }
    }
    output_data[output_size++] = test_board.info.max_shots;
    return output_size == 3001 ? 0 : -92;
}

int run_board_rle_writer_boundary(void) {
    static const uint8_t expected_counts[6] = {255, 255, 255, 255, 255, 225};
    const size_t board_start = 514;
    const size_t rle_start = board_start + 51;
    zoo_io_handle out_handle;
    size_t board_len;
    int ix, iy, ret;

    heap_used = 0;
    output_size = 0;
    memset(&state, 0, sizeof(state));
    memset(output_data, 0, sizeof(output_data));
    state.world.board_count = 0;
    state.world.info.current_board = 0;
    state.board.stat_count = 0;
    for (ix = 1; ix <= ZOO_BOARD_WIDTH; ix++) {
        for (iy = 1; iy <= ZOO_BOARD_HEIGHT; iy++) {
            state.board.tiles[ix][iy].element = ZOO_E_EMPTY;
            state.board.tiles[ix][iy].color = 0x70;
        }
    }

    ret = zoo_board_close(&state);
    if (ret != 0) return ret;
    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_world_write(&out_handle, &state.world);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    if (output_size < rle_start + sizeof(expected_counts) * 3) return -91;
    board_len = (size_t)output_data[512] | ((size_t)output_data[513] << 8);
    if (output_size != board_start + board_len) return -92;

    for (ix = 0; ix < 6; ix++) {
        size_t at = rle_start + (size_t)ix * 3;
        if (output_data[at] != expected_counts[ix]) return ix + 1;
        if (output_data[at + 1] != ZOO_E_EMPTY || output_data[at + 2] != 0x70) return ix + 10;
    }
    return 0;
}

static uint8_t brd_fixture[256];
static uint8_t brd_export[256];
static zoo_board brd_test_board;

int run_editor_brd_roundtrip(uint32_t offset, uint32_t board_len) {
    int result;
    if (offset > FIXTURE_CAP || board_len > FIXTURE_CAP - offset) return -90;
    heap_used = 0;
    output_size = 0;
    memset(&brd_test_board, 0, sizeof(brd_test_board));
    result = zoo_editor_brd_parse(input_data + offset, board_len, &brd_test_board);
    if (result != 0) return result;
    result = zoo_editor_brd_serialize(&brd_test_board, output_data, FIXTURE_CAP, &output_size);
    zoo_editor_brd_free(&brd_test_board);
    return result;
}

int run_editor_brd_record_roundtrip(uint32_t input_len) {
    int result;
    if (input_len < 2 || input_len > FIXTURE_CAP) return -90;
    heap_used = 0;
    output_size = 0;
    memset(&brd_test_board, 0, sizeof(brd_test_board));
    result = zoo_editor_brd_import_record(input_data, input_len, &brd_test_board);
    if (result != 0) return result;
    result = zoo_editor_brd_serialize(&brd_test_board, output_data, FIXTURE_CAP, &output_size);
    zoo_editor_brd_free(&brd_test_board);
    return result;
}

int run_editor_brd_signed_field_roundtrip(uint32_t input_len, int32_t expected_raw) {
    int result;
    int16_t expected;
    if (input_len < 2 || input_len > FIXTURE_CAP) return -90;
    if (expected_raw < -32768 || expected_raw > 32767) return -91;
    expected = (int16_t) expected_raw;
    heap_used = 0;
    output_size = 0;
    memset(&brd_test_board, 0, sizeof(brd_test_board));
    result = zoo_editor_brd_import_record(input_data, input_len, &brd_test_board);
    if (result != 0) return result;
    if (brd_test_board.info.time_limit_sec != expected) return 100;
    for (int i = 0; i <= brd_test_board.stat_count; i++) {
        const zoo_stat *stat = &brd_test_board.stats[i];
        if (stat->step_x != expected) return 200 + i;
        if (stat->step_y != expected) return 300 + i;
        if (stat->cycle != expected) return 400 + i;
        if (stat->follower != expected) return 500 + i;
        if (stat->leader != expected) return 600 + i;
        if (stat->data_pos != expected) return 700 + i;
    }
    result = zoo_editor_brd_serialize(&brd_test_board, output_data, FIXTURE_CAP, &output_size);
    zoo_editor_brd_free(&brd_test_board);
    return result;
}

int run_editor_brd_reject_truncated(void) {
    static const uint8_t partial_length[] = {0x04};
    static const uint8_t short_payload[] = {0x04, 0x00, 0x10, 0x20};
    zoo_board test_board;
    int result;

    memset(&test_board, 0, sizeof(test_board));
    result = zoo_editor_brd_import_record(partial_length, sizeof(partial_length), &test_board);
    if (result != ZOO_ERROR_INVAL) return 1;

    memset(&test_board, 0, sizeof(test_board));
    result = zoo_editor_brd_import_record(short_payload, sizeof(short_payload), &test_board);
    if (result != ZOO_ERROR_INVAL) return 2;
    return 0;
}

int run_editor_brd_transfer(uint32_t input_len) {
    int result;
    size_t board_length = 0;
    zoo_editor_brd_transfer_outcome outcome = ZOO_EDITOR_BRD_TRANSFER_IMPORTED;
    if (input_len > FIXTURE_CAP) return -90;

    heap_used = 0;
    output_size = 0;
    memset(&state, 0, sizeof(state));
    memset(output_data, 0, sizeof(output_data));
    zoo_world_create(&state);
    state.world.board_count = 1;
    state.world.info.current_board = 0;
    zoo_io_pstring_assign_pascal(state.board.name, sizeof(state.board.name) - 1,
            state.board.io_name_data, &state.board.io_name_length,
            &state.board.io_name_valid, "BEFORE", 6);
    state.board.info.max_shots = 41;
    state.board.info.neighbor_boards[0] = 1;
    state.board.info.neighbor_boards[1] = 2;
    state.board.info.neighbor_boards[2] = 3;
    state.board.info.neighbor_boards[3] = 4;
    state.board.tiles[30][12].element = ZOO_E_SOLID;
    state.board.tiles[30][12].color = 0x2F;
    result = zoo_board_close(&state);
    if (result != 0) return result;

    result = zoo_editor_brd_transfer(&state, input_len == 0 ? NULL : input_data,
            input_len, &outcome);
    if (result != 0) return result;
    output_data[0] = (uint8_t) outcome;
    output_data[1] = (uint8_t) state.world.board_len[0];
    output_data[2] = (uint8_t) ((uint16_t) state.world.board_len[0] >> 8);
    result = zoo_editor_brd_serialize(&state.board, output_data + 3,
            FIXTURE_CAP - 3, &board_length);
    if (result != 0) return result;
    output_size = board_length + 3;
    return 0;
}

int run_editor_brd_zero_count_rle(void) {
    size_t position = 51;
    size_t exported_length = 0;
    int remaining = ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT - 256;
    int result;
    static const uint8_t expected_counts[7] = {255, 1, 255, 255, 255, 255, 224};
    static const uint8_t expected_elements[7] = {
        ZOO_E_BREAKABLE, ZOO_E_BREAKABLE, ZOO_E_WATER, ZOO_E_WATER,
        ZOO_E_WATER, ZOO_E_WATER, ZOO_E_WATER
    };
    static const uint8_t expected_colors[7] = {0x2E, 0x2E, 0x31, 0x31, 0x31, 0x31, 0x31};
    if (sizeof(zoo_tile) != 2) return -90;

    memset(brd_fixture, 0, sizeof(brd_fixture));
    brd_fixture[position++] = 0;
    brd_fixture[position++] = ZOO_E_BREAKABLE;
    brd_fixture[position++] = 0x2E;
    while (remaining > 0) {
        uint8_t count = (uint8_t) (remaining > 255 ? 255 : remaining);
        brd_fixture[position++] = count;
        brd_fixture[position++] = ZOO_E_WATER;
        brd_fixture[position++] = 0x31;
        remaining -= count;
    }
    position += 86;
    position += 2;
    position += 33;
    brd_fixture[position++] = 0xA5;

    result = zoo_editor_brd_parse(brd_fixture, position, &brd_test_board);
    if (result != 0) return result;
    if (brd_test_board.tiles[1][1].element != ZOO_E_BREAKABLE
            || brd_test_board.tiles[1][1].color != 0x2E) return 1;
    if (brd_test_board.tiles[16][5].element != ZOO_E_BREAKABLE
            || brd_test_board.tiles[16][5].color != 0x2E) return 2;
    if (brd_test_board.tiles[17][5].element != ZOO_E_WATER
            || brd_test_board.tiles[17][5].color != 0x31) return 3;
    if (brd_test_board.tiles[60][25].element != ZOO_E_WATER
            || brd_test_board.tiles[60][25].color != 0x31) return 4;
    result = zoo_editor_brd_serialize(&brd_test_board, brd_export,
            sizeof(brd_export), &exported_length);
    if (result != 0 || exported_length != 51 + 7 * 3 + 86 + 2 + 33) return 5;
    position = 51;
    for (int i = 0; i < 7; i++) {
        if (brd_export[position++] != expected_counts[i]
                || brd_export[position++] != expected_elements[i]
                || brd_export[position++] != expected_colors[i]) return 6;
    }
    zoo_editor_brd_free(&brd_test_board);
    return 0;
}

int run_shared_stat_data_alias(void) {
    static char shared_data[] = {'A', 0, 'B', '\r', 'C', '\n', 0x1A, 0};
    int ret;
    zoo_io_handle in_handle, out_handle;

    heap_used = 0;
    output_size = 0;
    memset(&test_board, 0, sizeof(test_board));
    memset(output_data, 0, sizeof(output_data));
    strncpy(test_board.name, "Shared data alias", sizeof(test_board.name) - 1);
    test_board.stat_count = 2;
    test_board.stats[1].data = shared_data;
    test_board.stats[1].data_len = sizeof(shared_data);
    test_board.stats[2].data = shared_data;
    test_board.stats[2].data_len = sizeof(shared_data);

    out_handle = zoo_io_open_file_mem(output_data, FIXTURE_CAP, MODE_WRITE);
    ret = zoo_io_board_write(&out_handle, &test_board);
    if (ret != 0) return ret;
    output_size = (size_t)out_handle.func_tell(&out_handle);
    if (test_board.stats[2].data_len != -1 || output_size > FIXTURE_CAP) return -101;

    memcpy(input_data, output_data, output_size);
    heap_used = 0;
    memset(&test_board, 0, sizeof(test_board));
    in_handle = zoo_io_open_file_mem(input_data, output_size, MODE_READ);
    ret = zoo_io_board_read(&in_handle, &test_board);
    if (ret != 0) return ret;
    if (test_board.stats[1].data_len != (int16_t)sizeof(shared_data)
            || test_board.stats[2].data_len != (int16_t)sizeof(shared_data)
            || test_board.stats[1].data == NULL
            || test_board.stats[1].data != test_board.stats[2].data
            || memcmp(test_board.stats[1].data, shared_data, sizeof(shared_data)) != 0) return -102;
    return 0;
}
