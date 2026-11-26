#include <stddef.h>
#include <stdint.h>
#include <zoo.h>
#include <string.h>

#include "../../third_party/libzoo/src/libzoo/zoo.c"

static zoo_state state;
static zoo_io_driver driver;
static char opened_name[33];
static uint8_t fake_file;
static bool open_succeeds;
static int open_count;
static int close_count;
static int load_count;
static int32_t missing_reload_output[19];

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

size_t strlen(const char *str) {
    size_t length = 0;
    while (str[length] != '\0') length++;
    return length;
}

char *strncpy(char *dest, const char *src, size_t count) {
    size_t i;
    for (i = 0; i < count && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < count; i++) dest[i] = '\0';
    return dest;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) { left++; right++; }
    return (uint8_t) *left - (uint8_t) *right;
}

char *strncat(char *dest, const char *src, size_t count) {
    size_t offset = strlen(dest);
    size_t i = 0;
    while (i < count && src[i] != '\0') { dest[offset + i] = src[i]; i++; }
    dest[offset + i] = '\0';
    return dest;
}

static void close_fake_file(zoo_io_handle *handle) {
    (void) handle;
    close_count++;
}

static zoo_io_handle open_fake_file(zoo_io_driver *io, const char *filename, zoo_io_mode mode) {
    zoo_io_handle handle;
    (void) io; (void) mode;
    memset(&handle, 0, sizeof(handle));
    strncpy(opened_name, filename, sizeof(opened_name) - 1);
    open_count++;
    handle.p = open_succeeds ? &fake_file : NULL;
    handle.func_close = close_fake_file;
    return handle;
}

int zoo_world_load(zoo_state *s, zoo_io_handle *handle, bool title_only) {
    (void) handle; (void) title_only;
    load_count++;
    s->world.info.current_board = 7;
    return 0;
}

static void reset_case(void) {
    memset(&state, 0, sizeof(state));
    memset(&driver, 0, sizeof(driver));
    memset(opened_name, 0, sizeof(opened_name));
    driver.func_open_file = open_fake_file;
    state.d_io = &driver;
    open_count = 0;
    close_count = 0;
    load_count = 0;
}

int run_world_reload_checks(void) {
    int result;

    reset_case();
    open_succeeds = true;
    result = zoo_world_reload(&state);
    if (result != 0 || strcmp(opened_name, ".ZZT") != 0 || open_count != 1
            || load_count != 1 || close_count != 1 || state.return_board_id != 7) return 1;

    reset_case();
    open_succeeds = true;
    strncpy(state.world.info.name, "WORLD", sizeof(state.world.info.name) - 1);
    result = zoo_world_reload(&state);
    if (result != 0 || strcmp(opened_name, "WORLD.ZZT") != 0 || load_count != 1 || close_count != 1) return 2;

    reset_case();
    open_succeeds = false;
    result = zoo_world_reload(&state);
    if (result != ZOO_ERROR_INVAL || strcmp(opened_name, ".ZZT") != 0
            || load_count != 0 || close_count != 1 || state.return_board_id != 0) return 3;

    reset_case();
    state.d_io = NULL;
    result = zoo_world_reload(&state);
    if (result != ZOO_ERROR_INVAL || open_count != 0 || load_count != 0) return 4;

    reset_case();
    open_succeeds = true;
    state.world.info.name[0] = 'A';
    state.world.info.name[1] = '\0';
    state.world.info.name[2] = 'B';
    state.world.info.name[3] = '\0';
    result = zoo_world_reload(&state);
    if (result != 0 || strcmp(opened_name, "A.ZZT") != 0 || load_count != 1 || close_count != 1) return 5;

    reset_case();
    open_succeeds = true;
    state.world.info.name[0] = (char) 0x82;
    state.world.info.name[1] = 'C';
    state.world.info.name[2] = 'a';
    state.world.info.name[3] = 's';
    state.world.info.name[4] = 'e';
    state.world.info.name[5] = '\0';
    result = zoo_world_reload(&state);
    if (result != 0 || (uint8_t) opened_name[0] != 0x82
            || strcmp(opened_name + 1, "Case.ZZT") != 0 || load_count != 1 || close_count != 1) return 6;

    return 0;
}

int32_t run_world_reload_missing_capture(void) {
    int result;

    reset_case();
    open_succeeds = false;
    state.world.info.current_board = 0;
    state.world.info.is_save = true;
    state.world.info.ammo = 17;
    state.world.info.gems = 23;
    state.world.info.health = 37;
    state.world.info.torches = 2;
    state.world.info.torch_ticks = 123;
    state.world.info.energizer_ticks = 43;
    state.world.info.score = 777;
    strncpy(state.world.info.name, "MISSFILE", sizeof(state.world.info.name) - 1);
    state.return_board_id = 77;
    state.board.stat_count = 0;
    state.board.stats[0].x = 31;
    state.board.stats[0].y = 11;
    state.board.stats[0].cycle = 7;
    state.board.tiles[10][12].element = ZOO_E_SOLID;
    state.board.tiles[10][12].color = 0x2E;

    result = zoo_world_reload(&state);
    missing_reload_output[0] = result;
    missing_reload_output[1] = state.return_board_id;
    missing_reload_output[2] = state.world.board_count;
    missing_reload_output[3] = state.world.info.current_board;
    missing_reload_output[4] = state.world.info.is_save ? 1 : 0;
    missing_reload_output[5] = state.world.info.ammo;
    missing_reload_output[6] = state.world.info.gems;
    missing_reload_output[7] = state.world.info.health;
    missing_reload_output[8] = state.world.info.torches;
    missing_reload_output[9] = state.world.info.torch_ticks;
    missing_reload_output[10] = state.world.info.energizer_ticks;
    missing_reload_output[11] = state.world.info.score;
    missing_reload_output[12] = state.board.stat_count;
    missing_reload_output[13] = state.board.stats[0].x;
    missing_reload_output[14] = state.board.stats[0].y;
    missing_reload_output[15] = state.board.stats[0].cycle;
    missing_reload_output[16] = state.board.tiles[10][12].element;
    missing_reload_output[17] = state.board.tiles[10][12].color;
    missing_reload_output[18] = strcmp(opened_name, "MISSFILE.ZZT") == 0 ? 1 : 0;

    if (result != ZOO_ERROR_INVAL || state.return_board_id != 0
            || open_count != 1 || close_count != 1 || load_count != 0) return 1;
    return 0;
}

uint32_t world_reload_missing_output_ptr(void) {
    return (uint32_t) (uintptr_t) missing_reload_output;
}

int32_t world_reload_missing_output_count(void) {
    return 19;
}
