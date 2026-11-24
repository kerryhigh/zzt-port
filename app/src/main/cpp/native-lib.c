#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zoo.h"
#include "libzoo/zoo_internal.h"
#include "zoo_io_posix.h"
#include "zoo_sound_pcm.h"
#include "zoo_ui.h"
#include "zoo_editor_brd.h"
#include "zzt_frame_scheduler.h"

#define BOARD_CELLS (ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT)
#define BOARD_BYTES (BOARD_CELLS * 2)
#define FONT_BYTES (256 * 14)
#define TRANSITION_QUEUE_CAPACITY 8

extern unsigned char res_8x14_bin[];

static zoo_state game;
static zoo_ui_state ui;
static zoo_io_path_driver disk;
static zoo_sound_pcm_driver sound_pcm;
static zoo_video_driver video;
static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t board_frame[BOARD_BYTES];
static bool ready;
static bool modal_prompt_blocked;
static int last_error;
static zzt_frame_scheduler frame_scheduler;
typedef struct {
    uint8_t from[BOARD_BYTES];
    uint8_t to[BOARD_BYTES];
    uint16_t order[BOARD_CELLS];
    zoo_video_transition type;
} board_transition;
static board_transition transitions[TRANSITION_QUEUE_CAPACITY];
static unsigned transition_read_index;
static unsigned transition_count;
static int transition_capture_index = -1;
static bool editor_active;
static bool editor_was_modified;
static zoo_game_state editor_previous_game_state;
static bool editor_clipboard_valid;
static bool editor_clipboard_has_stat;
static zoo_tile editor_clipboard_tile;
static zoo_stat editor_clipboard_stat;

static void editor_clipboard_clear(void) {
    // Pascal's copiedStat is a shallow TStat copy, so Data is borrowed from a board stat.
    memset(&editor_clipboard_stat, 0, sizeof(editor_clipboard_stat));
    memset(&editor_clipboard_tile, 0, sizeof(editor_clipboard_tile));
    editor_clipboard_valid = false;
    editor_clipboard_has_stat = false;
}

typedef struct {
    uint8_t p1, p2, p3;
    int16_t step_x, step_y;
} editor_stat_setting;

static editor_stat_setting editor_stat_settings[ZOO_MAX_ELEMENT + 1];

static void editor_stat_settings_init(void) {
    for (int i = 0; i <= ZOO_MAX_ELEMENT; i++) {
        editor_stat_settings[i].p1 = 4;
        editor_stat_settings[i].p2 = 4;
        editor_stat_settings[i].p3 = 0;
        editor_stat_settings[i].step_x = 0;
        editor_stat_settings[i].step_y = -1;
    }
    editor_stat_settings[ZOO_E_OBJECT].p1 = 1;
    editor_stat_settings[ZOO_E_BEAR].p1 = 8;
}

static uint8_t editor_element_color(uint8_t element, uint8_t cursor_color) {
    uint8_t base = zoo_element_defs[element].color;
    if (base == ZOO_COLOR_CHOICE_ON_BLACK) return cursor_color;
    if (base == ZOO_COLOR_WHITE_ON_CHOICE) return (uint8_t) ((cursor_color * 0x10) - 0x71);
    if (base == ZOO_COLOR_CHOICE_ON_CHOICE) return (uint8_t) (((cursor_color - 8) * 0x11) + 8);
    return base;
}

static bool editor_prepare_tile(int16_t x, int16_t y) {
    // Pascal sets wasModified before BoardPrepareTileForPlacement, including
    // attempts to overwrite the player tile that the placement then rejects.
    editor_was_modified = true;
    int16_t stat_id = zoo_stat_get_id(&game, x, y);
    if (stat_id > 0) {
        zoo_stat_remove(&game, stat_id);
    } else if (stat_id == 0) {
        return false;
    } else if (!zoo_element_defs[game.board.tiles[x][y].element].placeable_on_top) {
        game.board.tiles[x][y].element = ZOO_E_EMPTY;
    }
    zoo_board_draw_tile(&game, x, y);
    return true;
}

static void editor_draw_tile_and_neighbors(int16_t x, int16_t y) {
    static const int16_t dx[4] = {0, 0, -1, 1};
    static const int16_t dy[4] = {-1, 1, 0, 0};
    zoo_board_draw_tile(&game, x, y);
    for (int i = 0; i < 4; i++) {
        int16_t nx = x + dx[i];
        int16_t ny = y + dy[i];
        if (nx >= 1 && nx <= ZOO_BOARD_WIDTH && ny >= 1 && ny <= ZOO_BOARD_HEIGHT) {
            zoo_board_draw_tile(&game, nx, ny);
        }
    }
}

static void video_write(zoo_video_driver *driver, int16_t x, int16_t y,
                        uint8_t color, uint8_t character) {
    (void) driver;
    if (x < 0 || x >= ZOO_BOARD_WIDTH || y < 0 || y >= ZOO_BOARD_HEIGHT) return;
    if (editor_active && game.board.tiles[x + 1][y + 1].element == ZOO_E_INVISIBLE) {
        color = ZOO_COLOR_CHOICE_ON_BLACK;
        character = 0xB0;
    }
    size_t offset = ((size_t) y * ZOO_BOARD_WIDTH + (size_t) x) * 2;
    board_frame[offset] = character;
    board_frame[offset + 1] = color;
}

static void video_transition_start(zoo_video_driver *driver) {
    (void) driver;
    if (transition_capture_index >= 0 || transition_count >= TRANSITION_QUEUE_CAPACITY) {
        transition_capture_index = -1;
        return;
    }

    unsigned slot = (transition_read_index + transition_count) % TRANSITION_QUEUE_CAPACITY;
    memcpy(transitions[slot].from, board_frame, BOARD_BYTES);
    transition_capture_index = (int) slot;
}

static void video_transition_finish(zoo_video_driver *driver,
                                    const uint16_t *order, uint16_t count,
                                    zoo_video_transition type) {
    (void) driver;
    if (transition_capture_index < 0) return;

    board_transition *transition = &transitions[transition_capture_index];
    memcpy(transition->to, board_frame, BOARD_BYTES);
    if (count > BOARD_CELLS) count = BOARD_CELLS;
    memcpy(transition->order, order, (size_t) count * sizeof(order[0]));
    transition->type = type;
    for (uint16_t i = count; i < BOARD_CELLS; i++) {
        transition->order[i] = i;
    }
    transition_count++;
    transition_capture_index = -1;
}

static void redraw(void) {
    if (ready) zoo_redraw(&game);
}

static int load_file(const char *filename, bool animate_picker_load) {
    if (!ready || filename == NULL || filename[0] == '\0') return ZOO_ERROR_INVAL;
    zoo_io_handle file = disk.parent.func_open_file(&disk.parent, filename, MODE_READ);
    // Pascal's WorldLoad calls Reset before WorldUnload. If opening fails,
    // DisplayIOError prevents the current world from being unloaded.
    if (file.p == NULL) {
        if (file.func_close != NULL) file.func_close(&file);
        last_error = ZOO_ERROR_INVAL;
        return last_error;
    }
    int result = zoo_world_load(&game, &file, false);
    if (file.func_close != NULL) file.func_close(&file);
    last_error = result;
    if (result != 0) return result;

    if (animate_picker_load) {
        zoo_board_draw_fill_transition(&game, 0x44);
    }
    if (game.world.info.is_save) {
        zoo_game_start(&game, GS_PLAY);
        zoo_board_draw_transition(&game);
    } else {
        zoo_board_change(&game, 0);
        zoo_game_start(&game, GS_TITLE);
        zoo_board_draw_transition(&game);
    }
    zoo_redraw(&game);
    return 0;
}

static int install_world_alias(const char *source_filename) {
    uint8_t target_name[sizeof(game.world.info.name) + 4];
    size_t target_length;
    const uint8_t *world_name;
    char source_path[ZOO_PATH_MAX + 1];
    char target_path[ZOO_PATH_MAX + 1];
    FILE *source;
    FILE *target;
    uint8_t buffer[32768];
    size_t count;
    int source_path_length;
    int target_path_length;
    int result = 0;

    if (!ready || source_filename == NULL || game.world.info.is_save) return ZOO_ERROR_INVAL;
    if (strchr(source_filename, '/') != NULL || strchr(source_filename, '\\') != NULL) {
        return ZOO_ERROR_INVAL;
    }

    // zoo_world_reload passes the C-string view of World.Info.Name to the
    // platform file API. In particular, a NUL inside the Pascal shortstring
    // truncates the reload path even though the serializer preserves its raw
    // length and trailing bytes. Build the alias from that same effective name.
    target_length = strnlen(game.world.info.name, sizeof(game.world.info.name) - 1);
    world_name = (const uint8_t *) game.world.info.name;
    if (target_length > sizeof(game.world.info.name) - 1) return ZOO_ERROR_INVAL;
    for (size_t i = 0; i < target_length; i++) {
        uint8_t c = world_name[i];
        // The alias must stay in this world's private directory. All other
        // CP437 bytes are valid POSIX filename bytes and remain significant.
        if (c == '/' || c == '\\') {
            return ZOO_ERROR_INVAL;
        }
        target_name[i] = c;
    }
    memcpy(target_name + target_length, ".ZZT", 4);
    target_name[target_length + 4] = '\0';

    if (strcmp(source_filename, (const char *) target_name) == 0) return 0;

    source_path_length = snprintf(source_path, sizeof(source_path), "%s/%s", disk.path, source_filename);
    target_path_length = snprintf(target_path, sizeof(target_path), "%s/%s", disk.path,
            (const char *) target_name);
    if (source_path_length < 0 || source_path_length >= (int) sizeof(source_path)
            || target_path_length < 0 || target_path_length >= (int) sizeof(target_path)) {
        return ZOO_ERROR_INVAL;
    }

    source = fopen(source_path, "rb");
    if (source == NULL) return ZOO_ERROR_INVAL;
    target = fopen(target_path, "wb");
    if (target == NULL) {
        fclose(source);
        return ZOO_ERROR_INVAL;
    }

    while ((count = fread(buffer, 1, sizeof(buffer), source)) != 0) {
        if (fwrite(buffer, 1, count, target) != count) {
            result = ZOO_ERROR_INVAL;
            break;
        }
    }
    if (ferror(source)) result = ZOO_ERROR_INVAL;
    if (fclose(target) != 0) result = ZOO_ERROR_INVAL;
    fclose(source);
    if (result != 0) remove(target_path);
    return result;
}

static const char *from_java(JNIEnv *env, jstring value, char *buffer, size_t capacity) {
    if (value == NULL || capacity == 0) return NULL;
    const char *utf = (*env)->GetStringUTFChars(env, value, NULL);
    if (utf == NULL) return NULL;
    strncpy(buffer, utf, capacity - 1);
    buffer[capacity - 1] = '\0';
    (*env)->ReleaseStringUTFChars(env, value, utf);
    return buffer;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_init(JNIEnv *env, jclass clazz,
                                  jstring world_directory, jstring initial_world) {
    (void) clazz;
    char directory[ZOO_PATH_MAX + 1];
    char filename[ZOO_PATH_MAX + 1];
    if (from_java(env, world_directory, directory, sizeof(directory)) == NULL
            || from_java(env, initial_world, filename, sizeof(filename)) == NULL) {
        return ZOO_ERROR_INVAL;
    }

    memset(&video, 0, sizeof(video));
    video.func_write = video_write;
    video.func_transition_start = video_transition_start;
    video.func_transition_finish = video_transition_finish;
    memset(board_frame, 0, sizeof(board_frame));
    memset(transitions, 0, sizeof(transitions));
    transition_read_index = 0;
    transition_count = 0;
    transition_capture_index = -1;
    zoo_state_init(&game);
    modal_prompt_blocked = false;
    editor_stat_settings_init();
    memset(&sound_pcm, 0, sizeof(sound_pcm));
    sound_pcm.frequency = 22050;
    sound_pcm.channels = 1;
    sound_pcm.volume = 200;
    sound_pcm.format_signed = false;
    sound_pcm.latency = 1;
    zoo_sound_pcm_init(&sound_pcm);
    game.sound.d_sound = &sound_pcm.parent;
    game.d_video = &video;
    zoo_io_create_posix_driver(&disk);
    strncpy(disk.path, directory, sizeof(disk.path) - 1);
    disk.path[sizeof(disk.path) - 1] = '\0';
    game.d_io = &disk.parent;
    zoo_ui_init(&ui, &game);
    ready = true;
    zzt_frame_scheduler_reset(&frame_scheduler);
    return load_file(filename, false);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_setWorldDirectory(JNIEnv *env, jclass clazz, jstring directory) {
    (void) clazz;
    if (!ready) return ZOO_ERROR_INVAL;
    char path[ZOO_PATH_MAX + 1];
    if (from_java(env, directory, path, sizeof(path)) == NULL) return ZOO_ERROR_INVAL;
    strncpy(disk.path, path, sizeof(disk.path) - 1);
    disk.path[sizeof(disk.path) - 1] = '\0';
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_installWorldAlias(JNIEnv *env, jclass clazz, jstring source_filename) {
    (void) clazz;
    char filename[ZOO_PATH_MAX + 1];
    if (from_java(env, source_filename, filename, sizeof(filename)) == NULL) return ZOO_ERROR_INVAL;
    return install_world_alias(filename);
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_worldFileExists(JNIEnv *env, jclass clazz, jbyteArray cp437_world_name) {
    (void) clazz;
    uint8_t filename[sizeof(game.world.info.name) + 4] = {0};
    if (!ready || cp437_world_name == NULL) return JNI_FALSE;

    jsize length = (*env)->GetArrayLength(env, cp437_world_name);
    if ((*env)->ExceptionCheck(env) || length > (jsize) (sizeof(game.world.info.name) - 1)) {
        return JNI_FALSE;
    }
    if (length > 0) {
        (*env)->GetByteArrayRegion(env, cp437_world_name, 0, length, (jbyte *) filename);
        if ((*env)->ExceptionCheck(env)) return JNI_FALSE;
    }

    size_t name_length = strnlen((const char *) filename, (size_t) length);
    for (size_t i = 0; i < name_length; i++) {
        if (filename[i] == '/' || filename[i] == '\\') return JNI_FALSE;
    }
    memcpy(filename + name_length, ".ZZT", 4);
    filename[name_length + 4] = '\0';

    zoo_io_handle file = disk.parent.func_open_file(&disk.parent, (const char *) filename, MODE_READ);
    jboolean exists = file.p != NULL ? JNI_TRUE : JNI_FALSE;
    if (file.func_close != NULL) file.func_close(&file);
    return exists;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_loadFile(JNIEnv *env, jclass clazz, jstring filename) {
    (void) clazz;
    char name[ZOO_PATH_MAX + 1];
    if (from_java(env, filename, name, sizeof(name)) == NULL) return ZOO_ERROR_INVAL;
    return load_file(name, true);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_startGame(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready) return ZOO_ERROR_INVAL;
    int result;
    if (game.game_state == GS_PLAY) {
        game.game_paused = false;
        result = 0;
    } else {
        result = zoo_world_play(&game);
    }
    last_error = result;
    zoo_redraw(&game);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_tickSpeed(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    return ready ? game.tick_speed : 4;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_setTickSpeed(JNIEnv *env, jclass clazz, jint speed) {
    (void) env; (void) clazz;
    if (!ready || speed < 0 || speed > 8) return;
    game.tick_speed = (int16_t) speed;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorBegin(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || editor_active || game.game_state != GS_TITLE) {
        return ZOO_ERROR_INVAL;
    }

    bool reset_world = game.world.info.is_save || zoo_flag_get_id(&game, "SECRET") >= 0;
    if (reset_world) {
        int result = zoo_world_close(&game);
        if (result != 0) return result;
        zoo_world_create(&game);
        editor_stat_settings_init();
    }

    editor_previous_game_state = game.game_state;
    editor_clipboard_clear();
    editor_active = true;
    editor_was_modified = false;
    game.game_state = GS_NONE;
    game.force_darkness_off = true;
    zoo_board_draw_ordered(&game);
    return reset_world ? 1 : 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorRefreshBoard(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || !editor_active) return ZOO_ERROR_INVAL;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorLoadWorldFile(JNIEnv *env, jclass clazz, jstring filename) {
    (void) clazz;
    if (!ready || !editor_active) return ZOO_ERROR_INVAL;
    char name[ZOO_PATH_MAX + 1];
    if (from_java(env, filename, name, sizeof(name)) == NULL || name[0] == '\0') {
        return ZOO_ERROR_INVAL;
    }
    zoo_io_handle file = disk.parent.func_open_file(&disk.parent, name, MODE_READ);
    int result = zoo_world_load(&game, &file, false);
    if (file.func_close != NULL) file.func_close(&file);
    last_error = result;
    if (result != 0) return result;
    editor_was_modified = false;

    bool reset_world = (game.world.info.is_save || zoo_flag_get_id(&game, "SECRET") >= 0)
            && !game.debug_enabled;
    if (reset_world) return 1;

    zoo_board_draw_fill_transition(&game, 0x44);
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorResetLoadedWorld(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || !editor_active) return ZOO_ERROR_INVAL;
    int result = zoo_world_close(&game);
    if (result != 0) return result;
    game.error_value = 0;
    zoo_world_create(&game);
    if (game.error_value != 0) return game.error_value;
    editor_stat_settings_init();
    game.current_stat_tick = game.board.stat_count + 1;
    game.game_paused = false;
    game.game_title_exit_requested = false;
    game.game_play_exit_requested = false;
    editor_was_modified = false;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorNewWorld(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || !editor_active) return ZOO_ERROR_INVAL;

    game.error_value = 0;
    int result = zoo_world_close(&game);
    if (result != 0) return result;
    zoo_world_create(&game);
    if (game.error_value != 0) return game.error_value;

    editor_stat_settings_init();
    game.current_tick = 0;
    game.current_stat_tick = game.board.stat_count + 1;
    game.game_tick_state = 0;
    game.game_paused = false;
    game.game_title_exit_requested = false;
    game.game_play_exit_requested = false;
    editor_was_modified = false;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_editorEnd(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || !editor_active) return;
    editor_clipboard_clear();
    editor_active = false;
    game.force_darkness_off = false;
    game.game_state = editor_previous_game_state;
    game.current_tick = game.func_random(&game, 100);
    game.current_stat_tick = game.board.stat_count + 1;
    game.game_tick_state = 0;
    zoo_redraw(&game);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorClearBoard(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || !editor_active) return ZOO_ERROR_INVAL;

    while (game.board.stat_count > 0) {
        zoo_stat_remove(&game, game.board.stat_count);
    }
    zoo_board_create(&game);
    game.error_value = 0;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorSwitchBoard(JNIEnv *env, jclass clazz, jint board_id) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || board_id < 0 || board_id > game.world.board_count) {
        return ZOO_ERROR_INVAL;
    }
    if (board_id == game.world.info.current_board) return 0;
    int result = zoo_board_close(&game);
    if (result != 0) return result;
    result = zoo_board_open(&game, (int16_t) board_id);
    if (result != 0) return result;
    game.error_value = 0;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorAppendBoard(JNIEnv *env, jclass clazz, jbyteArray board_name) {
    (void) clazz;
    if (!ready || !editor_active || board_name == NULL) return ZOO_ERROR_INVAL;
    jsize name_length = (*env)->GetArrayLength(env, board_name);
    if (name_length < 1 || name_length > 50 || game.world.board_count >= ZOO_MAX_BOARD) {
        return ZOO_ERROR_INVAL;
    }
    char name[51] = {0};
    (*env)->GetByteArrayRegion(env, board_name, 0, name_length, (jbyte *) name);
    if ((*env)->ExceptionCheck(env)) return ZOO_ERROR_INVAL;
    for (jsize i = 0; i < name_length; i++) {
        if (name[i] == '\0') return ZOO_ERROR_INVAL;
    }

    int16_t previous_board = game.world.info.current_board;
    int16_t previous_count = game.world.board_count;
    int result = zoo_board_close(&game);
    if (result != 0) return result;

    int16_t new_board = (int16_t) (previous_count + 1);
    game.world.board_count = new_board;
    game.world.info.current_board = new_board;
    game.world.board_data[new_board] = NULL;
    game.world.board_len[new_board] = 0;
    game.world.board_external[new_board] = false;
    zoo_board_create(&game);
    zoo_io_pstring_assign_pascal(game.board.name, sizeof(game.board.name) - 1,
            game.board.io_name_data, &game.board.io_name_length,
            &game.board.io_name_valid, name, (size_t) name_length);

    result = zoo_board_close(&game);
    if (result != 0) {
        game.world.board_count = previous_count;
        game.world.info.current_board = previous_board;
        zoo_board_open(&game, previous_board);
        return result;
    }
    result = zoo_board_open(&game, new_board);
    if (result != 0) {
        game.world.board_count = previous_count;
        game.world.info.current_board = previous_board;
        zoo_board_open(&game, previous_board);
        return result;
    }
    game.error_value = 0;
    zoo_board_draw_ordered(&game);
    return new_board;
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_editorBoardNameBytes(JNIEnv *env, jclass clazz, jint board_id) {
    (void) clazz;
    if (!ready || !editor_active || board_id < 0 || board_id > game.world.board_count) return NULL;
    const uint8_t *name_data = NULL;
    int name_length = 0;
    if (board_id == game.world.info.current_board) {
        name_data = (const uint8_t *) game.board.name;
        name_length = (int) strnlen(game.board.name, sizeof(game.board.name) - 1);
    } else {
        const uint8_t *board_data = (const uint8_t *) game.world.board_data[board_id];
        int board_length = game.world.board_len[board_id];
        if (board_data != NULL && board_length > 1) {
            name_length = board_data[0];
            if (name_length > 50) name_length = 50;
            if (name_length > board_length - 1) name_length = board_length - 1;
            name_data = board_data + 1;
        }
    }
    if (name_length < 0) name_length = 0;
    jbyteArray result = (*env)->NewByteArray(env, name_length);
    if (result != NULL && name_length > 0 && name_data != NULL) {
        (*env)->SetByteArrayRegion(env, result, 0, name_length, (const jbyte *) name_data);
    }
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_kerry_zzt_ZztNative_editorBoardInfo(JNIEnv *env, jclass clazz) {
    (void) clazz;
    if (!ready || !editor_active) return NULL;
    const zoo_board_info *info = &game.board.info;
    jint values[8] = {
        info->max_shots,
        info->is_dark ? 1 : 0,
        info->reenter_when_zapped ? 1 : 0,
        info->neighbor_boards[0],
        info->neighbor_boards[1],
        info->neighbor_boards[2],
        info->neighbor_boards[3],
        info->time_limit_sec
    };
    jintArray result = (*env)->NewIntArray(env, 8);
    if (result != NULL) (*env)->SetIntArrayRegion(env, result, 0, 8, values);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorUpdateBoardInfo(JNIEnv *env, jclass clazz, jbyteArray name,
        jint max_shots, jint is_dark, jint reenter_when_zapped, jint north, jint south,
        jint west, jint east, jint time_limit_sec) {
    (void) clazz;
    if (!ready || !editor_active || name == NULL || max_shots < 0 || max_shots > 255
            || (is_dark != 0 && is_dark != 1)
            || (reenter_when_zapped != 0 && reenter_when_zapped != 1)
            || north < 0 || north > 255 || south < 0 || south > 255
            || west < 0 || west > 255 || east < 0 || east > 255
            || time_limit_sec < -32768 || time_limit_sec > 32767) {
        return ZOO_ERROR_INVAL;
    }
    jsize name_length = (*env)->GetArrayLength(env, name);
    if (name_length < 0 || name_length > 50) return ZOO_ERROR_INVAL;
    char new_name[51] = {0};
    if (name_length > 0) {
        (*env)->GetByteArrayRegion(env, name, 0, name_length, (jbyte *) new_name);
        if ((*env)->ExceptionCheck(env)) return ZOO_ERROR_INVAL;
    }

    zoo_board *board = &game.board;
    bool same_name = zoo_io_pstring_matches_pascal(board->name, sizeof(board->name) - 1,
            board->io_name_data, board->io_name_length, board->io_name_valid,
            new_name, (size_t) name_length);
    if (!same_name) {
        for (jsize i = 0; i < name_length; i++) {
            if (new_name[i] == '\0') return ZOO_ERROR_INVAL;
        }
        zoo_io_pstring_assign_pascal(board->name, sizeof(board->name) - 1,
                board->io_name_data, &board->io_name_length, &board->io_name_valid,
                new_name, (size_t) name_length);
    }
    board->info.max_shots = (uint8_t) max_shots;
    board->info.is_dark = is_dark != 0;
    board->info.reenter_when_zapped = reenter_when_zapped != 0;
    board->info.neighbor_boards[0] = (uint8_t) north;
    board->info.neighbor_boards[1] = (uint8_t) south;
    board->info.neighbor_boards[2] = (uint8_t) west;
    board->info.neighbor_boards[3] = (uint8_t) east;
    board->info.time_limit_sec = (int16_t) time_limit_sec;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorImportBoard(JNIEnv *env, jclass clazz, jbyteArray source) {
    (void) clazz;
    if (!ready || !editor_active || source == NULL) return ZOO_ERROR_INVAL;
    jsize source_length = (*env)->GetArrayLength(env, source);
    if (source_length < 2) return ZOO_ERROR_INVAL;
    uint8_t length_bytes[2];
    (*env)->GetByteArrayRegion(env, source, 0, 2, (jbyte *) length_bytes);
    if ((*env)->ExceptionCheck(env)) return ZOO_ERROR_INVAL;
    uint16_t board_length = (uint16_t) (length_bytes[0] | ((uint16_t) length_bytes[1] << 8));
    if ((size_t) board_length > (size_t) source_length - 2) return ZOO_ERROR_INVAL;
    jsize record_length = (jsize) ((size_t) board_length + 2);
    uint8_t *bytes = (uint8_t *) malloc((size_t) record_length);
    if (bytes == NULL) return ZOO_ERROR_NOMEM;
    (*env)->GetByteArrayRegion(env, source, 0, record_length, (jbyte *) bytes);
    if ((*env)->ExceptionCheck(env)) {
        free(bytes);
        return ZOO_ERROR_INVAL;
    }

    zoo_board *replacement = (zoo_board *) calloc(1, sizeof(zoo_board));
    if (replacement == NULL) {
        free(bytes);
        return ZOO_ERROR_NOMEM;
    }
    int result = zoo_editor_brd_import_record(bytes, (size_t) record_length, replacement);
    free(bytes);
    if (result != 0) {
        zoo_editor_brd_free(replacement);
        free(replacement);
        return result;
    }
    result = zoo_board_close(&game);
    if (result != 0) {
        zoo_editor_brd_free(replacement);
        free(replacement);
        return result;
    }
    memset(replacement->info.neighbor_boards, 0, sizeof(replacement->info.neighbor_boards));
    memcpy(&game.board, replacement, sizeof(game.board));
    free(replacement);
    game.error_value = 0;
    zoo_board_draw_ordered(&game);
    return 0;
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_editorExportBoard(JNIEnv *env, jclass clazz) {
    (void) clazz;
    if (!ready || !editor_active) return NULL;
    uint8_t bytes[ZOO_MAX_BOARD_LEN + 2];
    size_t board_length = 0;
    int result = zoo_editor_brd_serialize(&game.board, bytes + 2, ZOO_MAX_BOARD_LEN, &board_length);
    if (result != 0 || board_length > ZOO_MAX_BOARD_LEN || board_length > 65535) return NULL;
    bytes[0] = (uint8_t) board_length;
    bytes[1] = (uint8_t) (board_length >> 8);
    jbyteArray output = (*env)->NewByteArray(env, (jsize) (board_length + 2));
    if (output != NULL) {
        (*env)->SetByteArrayRegion(env, output, 0, (jsize) (board_length + 2), (const jbyte *) bytes);
    }
    return output;
}

JNIEXPORT jobjectArray JNICALL
Java_com_kerry_zzt_ZztNative_editorElements(JNIEnv *env, jclass clazz, jint category) {
    (void) clazz;
    if (!ready) return NULL;
    int count = 0;
    for (int i = 1; i <= ZOO_MAX_ELEMENT; i++) {
        if (zoo_element_defs[i].editor_category == category) count++;
    }
    jclass string_class = (*env)->FindClass(env, "java/lang/String");
    if (string_class == NULL) return NULL;
    jobjectArray result = (*env)->NewObjectArray(env, count, string_class, NULL);
    if (result == NULL) return NULL;
    int out = 0;
    for (int i = 1; i <= ZOO_MAX_ELEMENT; i++) {
        const zoo_element_def *element = &zoo_element_defs[i];
        if (element->editor_category != category) continue;
        char item[80];
        snprintf(item, sizeof(item), "%d|%s", i, element->name);
        jstring value = (*env)->NewStringUTF(env, item);
        if (value == NULL) return NULL;
        (*env)->SetObjectArrayElement(env, result, out++, value);
        (*env)->DeleteLocalRef(env, value);
    }
    return result;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorElementByShortcut(JNIEnv *env, jclass clazz,
                                                       jint category, jint shortcut) {
    (void) env; (void) clazz;
    if (!ready || shortcut < 0 || shortcut > 0x7f) return -1;
    if (shortcut >= 'a' && shortcut <= 'z') shortcut -= ('a' - 'A');
    for (int i = 1; i <= ZOO_MAX_ELEMENT; i++) {
        const zoo_element_def *element = &zoo_element_defs[i];
        if (element->editor_category == category
                && element->editor_shortcut == shortcut) return i;
    }
    return -1;
}

JNIEXPORT jobjectArray JNICALL
Java_com_kerry_zzt_ZztNative_editorElementParameters(JNIEnv *env, jclass clazz, jint element_id) {
    (void) clazz;
    if (!ready || element_id < 0 || element_id > ZOO_MAX_ELEMENT) return NULL;
    const zoo_element_def *element = &zoo_element_defs[element_id];
    const char *fields[6] = {
        element->param1_name,
        element->param2_name,
        element->param_bullet_type_name,
        element->param_board_name,
        element->param_dir_name,
        element->param_text_name
    };
    jclass string_class = (*env)->FindClass(env, "java/lang/String");
    if (string_class == NULL) return NULL;
    jobjectArray result = (*env)->NewObjectArray(env, 6, string_class, NULL);
    if (result == NULL) return NULL;
    for (int i = 0; i < 6; i++) {
        jstring value = (*env)->NewStringUTF(env, fields[i]);
        if (value == NULL) return NULL;
        (*env)->SetObjectArrayElement(env, result, i, value);
        (*env)->DeleteLocalRef(env, value);
    }
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_kerry_zzt_ZztNative_editorStatValues(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return NULL;
    }
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id < 0) return NULL;
    const zoo_stat *stat = &game.board.stats[stat_id];
    jint values[7] = {
        stat_id,
        game.board.tiles[x][y].element,
        stat->p1,
        stat->p2,
        stat->p3,
        stat->step_x,
        stat->step_y
    };
    jintArray result = (*env)->NewIntArray(env, 7);
    if (result != NULL) (*env)->SetIntArrayRegion(env, result, 0, 7, values);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorUpdateStat(JNIEnv *env, jclass clazz, jint x, jint y,
        jint p1, jint p2, jint p3, jint step_x, jint step_y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT
            || p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255 || p3 < 0 || p3 > 255
            || step_x < -1 || step_x > 1 || step_y < -1 || step_y > 1
            || (step_x != 0 && step_y != 0)) {
        return ZOO_ERROR_INVAL;
    }
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id <= 0) return ZOO_ERROR_INVAL;
    uint8_t element_id = game.board.tiles[x][y].element;
    const zoo_element_def *element = &zoo_element_defs[element_id];
    zoo_stat *stat = &game.board.stats[stat_id];
    stat->p1 = (uint8_t) p1;
    stat->p2 = (uint8_t) p2;
    stat->p3 = (uint8_t) p3;
    stat->step_x = (int16_t) step_x;
    stat->step_y = (int16_t) step_y;
    if (element->param1_name[0] != '\0') editor_stat_settings[element_id].p1 = stat->p1;
    if (element->param2_name[0] != '\0' || element->param_bullet_type_name[0] != '\0') {
        editor_stat_settings[element_id].p2 = stat->p2;
    }
    if (element->param_board_name[0] != '\0') editor_stat_settings[element_id].p3 = stat->p3;
    if (element->param_dir_name[0] != '\0') {
        editor_stat_settings[element_id].step_x = stat->step_x;
        editor_stat_settings[element_id].step_y = stat->step_y;
    }
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_editorStatText(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return NULL;
    }
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id <= 0) return NULL;
    zoo_stat *stat = &game.board.stats[stat_id];
    uint8_t element_id = game.board.tiles[x][y].element;
    if (zoo_element_defs[element_id].param_text_name[0] == '\0') return NULL;
    jsize length = stat->data_len > 0 ? stat->data_len : 0;
    jbyteArray result = (*env)->NewByteArray(env, length);
    if (result != NULL && length > 0 && stat->data != NULL) {
        (*env)->SetByteArrayRegion(env, result, 0, length, (const jbyte *) stat->data);
    }
    return result;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorUpdateStatText(JNIEnv *env, jclass clazz, jint x, jint y,
        jbyteArray text) {
    (void) clazz;
    if (!ready || !editor_active || text == NULL || x < 1 || x > ZOO_BOARD_WIDTH
            || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    jsize length = (*env)->GetArrayLength(env, text);
    if (length < 0 || length > 32767) return ZOO_ERROR_INVAL;
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id <= 0) return ZOO_ERROR_INVAL;
    uint8_t element_id = game.board.tiles[x][y].element;
    if (zoo_element_defs[element_id].param_text_name[0] == '\0') return ZOO_ERROR_INVAL;

    char *replacement = NULL;
    if (length > 0) {
        replacement = (char *) malloc((size_t) length);
        if (replacement == NULL) return ZOO_ERROR_NOMEM;
        (*env)->GetByteArrayRegion(env, text, 0, length, (jbyte *) replacement);
        if ((*env)->ExceptionCheck(env)) {
            free(replacement);
            return ZOO_ERROR_INVAL;
        }
    }

    zoo_stat *stat = &game.board.stats[stat_id];
    char *old_data = stat->data;
    bool shared = false;
    if (old_data != NULL) {
        for (int i = 1; i <= game.board.stat_count; i++) {
            if (i != stat_id && game.board.stats[i].data == old_data) {
                shared = true;
                break;
            }
        }
    }
    if (!shared && stat->data_in_heap) {
        zoo_board_data_heap_release(&game.board, old_data, 0);
    }
    if (shared) stat->data = NULL;
    zoo_stat_free(stat);
    stat->data = replacement;
    stat->data_len = (int16_t) length;
    stat->data_in_heap = false;
    stat->data_heap_offset = 0;
    stat->data_reuse_pending = false;
    stat->data_reuse_order = 0;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorRemoveStat(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id <= 0) return ZOO_ERROR_INVAL;
    zoo_stat_remove(&game, stat_id);
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

static int editor_place_element(int16_t x, int16_t y, uint8_t element, uint8_t cursor_color) {
    if (element != ZOO_E_PLAYER && zoo_element_defs[element].cycle != -1
            && game.board.stat_count >= ZOO_MAX_STAT) return 1;
    if (!editor_prepare_tile(x, y)) return 1;

    if (element == ZOO_E_PLAYER) {
        zoo_stat_move(&game, 0, x, y);
    } else if (zoo_element_defs[element].cycle == -1) {
        game.board.tiles[x][y].element = (uint8_t) element;
        game.board.tiles[x][y].color = editor_element_color(element, cursor_color);
        zoo_board_draw_tile(&game, x, y);
    } else {
        zoo_stat defaults = zoo_stat_template_default;
        const zoo_element_def *definition = &zoo_element_defs[element];
        if (definition->param1_name[0] != '\0') defaults.p1 = editor_stat_settings[element].p1;
        if (definition->param2_name[0] != '\0' || definition->param_bullet_type_name[0] != '\0') {
            defaults.p2 = editor_stat_settings[element].p2;
        }
        if (definition->param_board_name[0] != '\0') defaults.p3 = editor_stat_settings[element].p3;
        if (definition->param_dir_name[0] != '\0') {
            defaults.step_x = editor_stat_settings[element].step_x;
            defaults.step_y = editor_stat_settings[element].step_y;
        }
        zoo_stat_add(&game, x, y, element, editor_element_color(element, cursor_color),
                zoo_element_defs[element].cycle, &defaults);
    }
    editor_draw_tile_and_neighbors(x, y);
    return 0;
}

static int editor_place_pattern_element(int16_t x, int16_t y, uint8_t element,
                                        uint8_t cursor_color) {
    if (zoo_element_defs[element].cycle != -1) {
        return editor_place_element(x, y, element, cursor_color);
    }
    if (!editor_prepare_tile(x, y)) return 1;
    game.board.tiles[x][y].element = element;
    game.board.tiles[x][y].color = cursor_color;
    editor_draw_tile_and_neighbors(x, y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPlace(JNIEnv *env, jclass clazz,
        jint x, jint y, jint element, jint cursor_color, jboolean use_pattern_color) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT
            || element < 0 || element > ZOO_MAX_ELEMENT || cursor_color < 0 || cursor_color > 255) {
        return ZOO_ERROR_INVAL;
    }
    if (use_pattern_color == JNI_TRUE) {
        return editor_place_pattern_element((int16_t) x, (int16_t) y,
                (uint8_t) element, (uint8_t) cursor_color);
    }
    return editor_place_element((int16_t) x, (int16_t) y, (uint8_t) element,
            (uint8_t) cursor_color);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPlot(JNIEnv *env, jclass clazz,
        jint x, jint y, jint element, jint cursor_color, jboolean use_pattern_color) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT
            || element < 0 || element > ZOO_MAX_ELEMENT || cursor_color < 0 || cursor_color > 255) {
        return ZOO_ERROR_INVAL;
    }

    int16_t cell_x = (int16_t) x;
    int16_t cell_y = (int16_t) y;
    if (game.board.tiles[cell_x][cell_y].element == ZOO_E_EMPTY) {
        if (use_pattern_color == JNI_TRUE) {
            return editor_place_pattern_element(cell_x, cell_y,
                    (uint8_t) element, (uint8_t) cursor_color);
        }
        return editor_place_element(cell_x, cell_y, (uint8_t) element, (uint8_t) cursor_color);
    }
    if (!editor_prepare_tile(cell_x, cell_y)) return 1;
    game.board.tiles[cell_x][cell_y].element = ZOO_E_EMPTY;
    editor_draw_tile_and_neighbors(cell_x, cell_y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPrepareTile(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    if (!editor_prepare_tile((int16_t) x, (int16_t) y)) return 1;
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorCopyTile(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }

    zoo_tile copied_tile = game.board.tiles[x][y];
    zoo_stat copied_stat;
    memset(&copied_stat, 0, sizeof(copied_stat));
    bool copied_has_stat = false;
    int16_t stat_id = zoo_stat_get_id(&game, (int16_t) x, (int16_t) y);
    if (stat_id >= 0) {
        const zoo_stat *source = &game.board.stats[stat_id];
        if (source->data_len < 0 || (source->data_len > 0 && source->data == NULL)) {
            return ZOO_ERROR_INVAL;
        }
        // Keep Pascal's shallow copiedStat.Data alias; AddStat copies bytes on paste.
        copied_stat = *source;
#ifdef ZOO_USE_LABEL_CACHE
        copied_stat.label_cache = NULL;
        copied_stat.label_cache_size = 0;
#ifdef ZOO_NO_OBJECT_CODE_WRITES
        copied_stat.label_cache_chr2 = 0;
#endif
#endif
        copied_has_stat = true;
    }

    editor_clipboard_clear();
    editor_clipboard_tile = copied_tile;
    editor_clipboard_stat = copied_stat;
    editor_clipboard_valid = true;
    editor_clipboard_has_stat = copied_has_stat;
    return 0;
}

static int editor_paste_tile(int16_t x, int16_t y) {
    if (editor_clipboard_has_stat && game.board.stat_count >= ZOO_MAX_STAT) return 1;
    if (!editor_prepare_tile(x, y)) return 1;

    if (editor_clipboard_has_stat) {
        zoo_stat_add(&game, x, y, editor_clipboard_tile.element,
                editor_clipboard_tile.color, editor_clipboard_stat.cycle, &editor_clipboard_stat);
    } else {
        game.board.tiles[x][y] = editor_clipboard_tile;
    }
    editor_draw_tile_and_neighbors(x, y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPasteTile(JNIEnv *env, jclass clazz, jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH
            || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    if (!editor_clipboard_valid) return 2;
    return editor_paste_tile((int16_t) x, (int16_t) y);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPlaceClipboardPattern(JNIEnv *env, jclass clazz,
                                                          jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH
            || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    if (editor_clipboard_valid) return editor_paste_tile((int16_t) x, (int16_t) y);
    if (!editor_prepare_tile((int16_t) x, (int16_t) y)) return 1;
    game.board.tiles[x][y].element = ZOO_E_EMPTY;
    game.board.tiles[x][y].color = 0x0f;
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPlotClipboardPattern(JNIEnv *env, jclass clazz,
                                                         jint x, jint y) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH
            || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }
    uint8_t element = game.board.tiles[x][y].element;
    if (element == ZOO_E_EMPTY
            || (zoo_element_defs[element].placeable_on_top && editor_clipboard_has_stat)) {
        return Java_com_kerry_zzt_ZztNative_editorPlaceClipboardPattern(env, clazz, x, y);
    }
    if (!editor_prepare_tile((int16_t) x, (int16_t) y)) return 1;
    game.board.tiles[x][y].element = ZOO_E_EMPTY;
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorPlaceText(JNIEnv *env, jclass clazz, jint x, jint y,
        jint character, jint cursor_color) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT
            || character < 32 || character >= 128 || cursor_color < 9 || cursor_color > 15) {
        return ZOO_ERROR_INVAL;
    }
    if (!editor_prepare_tile((int16_t) x, (int16_t) y)) return 1;
    game.board.tiles[x][y].element = (uint8_t) (ZOO_E_TEXT_MIN + cursor_color - 9);
    game.board.tiles[x][y].color = (uint8_t) character;
    editor_draw_tile_and_neighbors((int16_t) x, (int16_t) y);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_editorFloodFill(JNIEnv *env, jclass clazz, jint x, jint y,
        jint element, jint cursor_color, jboolean use_clipboard, jboolean use_pattern_color) {
    (void) env; (void) clazz;
    if (!ready || !editor_active || x < 1 || x > ZOO_BOARD_WIDTH || y < 1 || y > ZOO_BOARD_HEIGHT
            || element < 0 || element > ZOO_MAX_ELEMENT || cursor_color < 0 || cursor_color > 255
            || (!use_clipboard && element == ZOO_E_PLAYER)
            || (use_clipboard && !editor_clipboard_valid)) {
        return ZOO_ERROR_INVAL;
    }

    static const int16_t dx[4] = {0, 0, -1, 1};
    static const int16_t dy[4] = {-1, 1, 0, 0};
    // Match the reconstructed Pascal editor's 256-entry queue and byte counters.
    uint8_t queue_x[256] = {0};
    uint8_t queue_y[256] = {0};
    uint8_t to_fill = 1;
    uint8_t filled = 0;
    int16_t cell_x = (int16_t) x;
    int16_t cell_y = (int16_t) y;
    int changed = 0;
    zoo_tile source = game.board.tiles[x][y];

    while (to_fill != filled) {
        zoo_tile before = game.board.tiles[cell_x][cell_y];
        int result = use_clipboard
                ? editor_paste_tile(cell_x, cell_y)
                : (use_pattern_color == JNI_TRUE
                        ? editor_place_pattern_element(cell_x, cell_y,
                                (uint8_t) element, (uint8_t) cursor_color)
                        : editor_place_element(cell_x, cell_y,
                                (uint8_t) element, (uint8_t) cursor_color));
        if (result < 0) return result;
        zoo_tile after = game.board.tiles[cell_x][cell_y];
        if (result == 0 && (before.element != after.element || before.color != after.color)) {
            changed++;
            for (int i = 0; i < 4; i++) {
                int16_t next_x = cell_x + dx[i];
                int16_t next_y = cell_y + dy[i];
                if (next_x < 0 || next_x > ZOO_BOARD_WIDTH + 1
                        || next_y < 0 || next_y > ZOO_BOARD_HEIGHT + 1) {
                    continue;
                }
                zoo_tile candidate = game.board.tiles[next_x][next_y];
                if (candidate.element == source.element
                        && (source.element == ZOO_E_EMPTY || candidate.color == source.color)) {
                    queue_x[to_fill] = (uint8_t) next_x;
                    queue_y[to_fill] = (uint8_t) next_y;
                    to_fill = (uint8_t) (to_fill + 1);
                }
            }
        }
        filled = (uint8_t) (filled + 1);
        cell_x = queue_x[filled];
        cell_y = queue_y[filled];
    }
    return changed;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_returnToTitle(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready) return ZOO_ERROR_INVAL;
    int result = zoo_world_return_title(&game);
    last_error = result;
    return result;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_setMonochrome(JNIEnv *env, jclass clazz, jboolean enabled) {
    (void) env; (void) clazz;
    if (!ready) return;
    video.monochrome = enabled == JNI_TRUE;
    transition_read_index = 0;
    transition_count = 0;
    transition_capture_index = -1;
    zoo_redraw(&game);
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_setModalPromptBlocked(JNIEnv *env, jclass clazz, jboolean blocked) {
    (void) env; (void) clazz;
    if (ready) modal_prompt_blocked = blocked == JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_togglePause(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (ready && game.game_state == GS_PLAY) {
        game.game_paused = !game.game_paused;
        if (!game.game_paused) {
            game.current_tick = game.func_random(&game, 100);
            game.current_stat_tick = game.board.stat_count + 1;
        }
        zoo_redraw(&game);
    }
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_pauseGame(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (ready && game.game_state == GS_PLAY && game.world.info.health > 0) {
        game.game_paused = true;
        zoo_redraw(&game);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_gameCommandsAvailable(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    return ready && !editor_active && game.game_state == GS_PLAY
            && game.call_stack.call == NULL ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_gameHasMonitor(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || editor_active || game.game_state != GS_PLAY) return JNI_FALSE;
    for (int i = 1; i <= game.board.stat_count; i++) {
        zoo_stat *stat = &game.board.stats[i];
        if (game.board.tiles[stat->x][stat->y].element == ZOO_E_MONITOR) return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_debugCommand(JNIEnv *env, jclass clazz, jbyteArray command) {
    (void) clazz;
    if (!ready || (game.game_state != GS_TITLE && game.game_state != GS_PLAY)
            || game.call_stack.call != NULL || command == NULL) return;

    jsize length = (*env)->GetArrayLength(env, command);
    if (length > 50) return;
    char input[51] = {0};
    if (length > 0) {
        (*env)->GetByteArrayRegion(env, command, 0, length, (jbyte *) input);
        if ((*env)->ExceptionCheck(env)) return;
    }

    pthread_mutex_lock(&sound_mutex);
    zoo_game_debug_command(&game, input);
    pthread_mutex_unlock(&sound_mutex);
    zoo_redraw(&game);
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_toggleSound(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready) return JNI_FALSE;
    pthread_mutex_lock(&sound_mutex);
    game.sound.enabled = !game.sound.enabled;
    zoo_sound_clear_queue(&game.sound);
    bool enabled = game.sound.enabled;
    pthread_mutex_unlock(&sound_mutex);
    zoo_redraw(&game);
    return enabled ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_queueSoundToggle(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (!ready || game.game_state != GS_PLAY) return JNI_FALSE;
    bool enabled = !game.sound.enabled;
    zoo_ui_input_key(&game, &ui.input, 'b', true);
    zoo_ui_input_key(&game, &ui.input, 'b', false);
    return enabled ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_setWorldName(JNIEnv *env, jclass clazz, jbyteArray world_name) {
    (void) clazz;
    if (!ready || !editor_active || world_name == NULL) return ZOO_ERROR_INVAL;
    jsize length = (*env)->GetArrayLength(env, world_name);
    if ((*env)->ExceptionCheck(env)) return ZOO_ERROR_INVAL;
    size_t copy_length = (size_t) length;
    if (copy_length > sizeof(game.world.info.name) - 1) {
        copy_length = sizeof(game.world.info.name) - 1;
    }
    char name[sizeof(game.world.info.name)] = {0};
    if (copy_length > 0) {
        (*env)->GetByteArrayRegion(env, world_name, 0, (jsize) copy_length,
                (jbyte *) name);
        if ((*env)->ExceptionCheck(env)) return ZOO_ERROR_INVAL;
    }
    zoo_io_pstring_assign_pascal(game.world.info.name, sizeof(game.world.info.name) - 1,
            game.world.info.io_name_data, &game.world.info.io_name_length,
            &game.world.info.io_name_valid, name, copy_length);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_saveWorld(JNIEnv *env, jclass clazz, jstring filename) {
    (void) clazz;
    if (!ready) return ZOO_ERROR_INVAL;
    char name[ZOO_PATH_MAX + 1];
    if (from_java(env, filename, name, sizeof(name)) == NULL) return ZOO_ERROR_INVAL;
    zoo_io_handle file = disk.parent.func_open_file(&disk.parent, name, MODE_WRITE);
    int result = zoo_world_save(&game, &file);
    if (file.func_close != NULL) file.func_close(&file);
    last_error = result;
    if (result == 0 && editor_active) editor_was_modified = false;
    redraw();
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_kerry_zzt_ZztNative_editorWasModified(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    return editor_active && editor_was_modified ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_key(JNIEnv *env, jclass clazz, jint key, jboolean down) {
    (void) env; (void) clazz;
    if (!ready || key <= 0 || key > 0xffff) return;
    zoo_ui_input_key(&game, &ui.input, (uint16_t) key, down == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_keyStroke(JNIEnv *env, jclass clazz, jint key) {
    (void) env; (void) clazz;
    if (!ready || key <= 0 || key > 0xffff) return;
    zoo_ui_input_key(&game, &ui.input, (uint16_t) key, true);
    zoo_ui_input_key(&game, &ui.input, (uint16_t) key, false);
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_action(JNIEnv *env, jclass clazz, jint action, jboolean down) {
    (void) env; (void) clazz;
    if (!ready || action < 0 || action >= ZOO_ACTION_MAX) return;
    zoo_input_action_set(&game.input, (zoo_input_action) action, down == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_shootTap(JNIEnv *env, jclass clazz) {
    (void) env; (void) clazz;
    if (ready && !editor_active && game.game_state == GS_PLAY
            && !game.game_paused
            && game.call_stack.call == NULL) {
        game.input.shoot_tap = true;
    }
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_frame(JNIEnv *env, jclass clazz, jlong frame_time_nanos) {
	double now_ms;
	int pit_ticks;
    (void) env; (void) clazz;
    if (!ready) return;
	pit_ticks = zzt_frame_scheduler_begin_frame(&frame_scheduler,
		(int64_t) frame_time_nanos, &now_ms);
	while (pit_ticks-- > 0) {
        pthread_mutex_lock(&sound_mutex);
        zoo_tick_advance_pit(&game);
        zoo_sound_tick(&game.sound);
        zoo_sound_pcm_tick(&sound_pcm);
        pthread_mutex_unlock(&sound_mutex);
        zoo_input_tick(&game.input);
    }

    if (modal_prompt_blocked) {
		zzt_frame_scheduler_modal_prompt(&frame_scheduler, now_ms);
        return;
    }
    zoo_ui_tick(&ui);
	if (!zzt_frame_scheduler_game_due(&frame_scheduler, now_ms)) return;
    for (int i = 0; i < 1024; i++) {
        pthread_mutex_lock(&sound_mutex);
        zoo_tick_retval result = zoo_tick(&game);
        pthread_mutex_unlock(&sound_mutex);
        if (result == RETURN_IMMEDIATE) continue;
		if (result == ERROR) last_error = game.error_value;
		zzt_frame_scheduler_tick_finished(&frame_scheduler, now_ms, result);
        break;
    }
    if (game.game_play_exit_requested) {
        int result = zoo_world_return_title(&game);
        if (result != 0) last_error = result;
    }
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_copyBoard(JNIEnv *env, jclass clazz, jbyteArray target) {
    (void) clazz;
    if (!ready || target == NULL || (*env)->GetArrayLength(env, target) < BOARD_BYTES) return;
    (*env)->SetByteArrayRegion(env, target, 0, BOARD_BYTES, (const jbyte *) board_frame);
}

JNIEXPORT jint JNICALL
Java_com_kerry_zzt_ZztNative_takeTransition(JNIEnv *env, jclass clazz,
                                             jbyteArray from, jbyteArray to,
                                             jshortArray order) {
    (void) clazz;
    if (!ready || from == NULL || to == NULL || order == NULL
            || (*env)->GetArrayLength(env, from) < BOARD_BYTES
            || (*env)->GetArrayLength(env, to) < BOARD_BYTES
            || (*env)->GetArrayLength(env, order) < BOARD_CELLS
            || transition_count == 0) {
        return 0;
    }

    board_transition *transition = &transitions[transition_read_index];
    (*env)->SetByteArrayRegion(env, from, 0, BOARD_BYTES, (const jbyte *) transition->from);
    (*env)->SetByteArrayRegion(env, to, 0, BOARD_BYTES, (const jbyte *) transition->to);
    (*env)->SetShortArrayRegion(env, order, 0, BOARD_CELLS, (const jshort *) transition->order);
    if ((*env)->ExceptionCheck(env)) return 0;

    zoo_video_transition type = transition->type;
    transition_read_index = (transition_read_index + 1) % TRANSITION_QUEUE_CAPACITY;
    transition_count--;
    return (jint) type;
}

JNIEXPORT jintArray JNICALL
Java_com_kerry_zzt_ZztNative_status(JNIEnv *env, jclass clazz) {
    (void) clazz;
    jint values[19] = {0};
    if (ready) {
        uint8_t keys = 0;
        for (int i = 0; i < 7; i++) if (game.world.info.keys[i]) keys |= (uint8_t) (1u << i);
        values[0] = game.game_state;
        values[1] = game.world.info.health;
        values[2] = game.world.info.ammo;
        values[3] = game.world.info.gems;
        values[4] = game.world.info.torches;
        values[5] = game.world.info.score;
        values[6] = game.world.info.current_board;
        values[7] = game.world.board_count;
        values[8] = game.board.info.is_dark ? 1 : 0;
        values[9] = game.world.info.torch_ticks;
        values[10] = game.world.info.energizer_ticks;
        values[11] = keys;
        values[12] = game.board.stats[0].x;
        values[13] = game.board.stats[0].y;
        values[14] = game.game_paused ? 1 : 0;
        values[15] = game.world.info.is_save ? 1 : 0;
        values[16] = game.world.info.board_time_sec;
        values[17] = last_error;
        values[18] = game.board.info.time_limit_sec;
    }
    jintArray result = (*env)->NewIntArray(env, 19);
    if (result != NULL) (*env)->SetIntArrayRegion(env, result, 0, 19, values);
    return result;
}

static jbyteArray to_java_cp437_bytes(JNIEnv *env, const char *text, size_t capacity,
                                      bool raw_valid, uint8_t raw_length,
                                      const uint8_t *raw_data, size_t raw_capacity) {
    size_t length = 0;
    const char *bytes = text;
    if (raw_valid) {
        bytes = (const char *) raw_data;
        length = raw_length < raw_capacity ? raw_length : raw_capacity;
    } else {
        while (length < capacity && text[length] != '\0') length++;
    }
    jbyteArray result = (*env)->NewByteArray(env, (jsize) length);
    if (result != NULL && length > 0) {
        (*env)->SetByteArrayRegion(env, result, 0, (jsize) length, (const jbyte *) bytes);
    }
    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_worldNameBytes(JNIEnv *env, jclass clazz) {
    (void) clazz;
    return to_java_cp437_bytes(env, ready ? game.world.info.name : "", sizeof(game.world.info.name) - 1,
            ready && game.world.info.io_name_valid, game.world.info.io_name_length,
            game.world.info.io_name_data, sizeof(game.world.info.io_name_data));
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_boardNameBytes(JNIEnv *env, jclass clazz) {
    (void) clazz;
    return to_java_cp437_bytes(env, ready ? game.board.name : "", sizeof(game.board.name) - 1,
            ready && game.board.io_name_valid, game.board.io_name_length,
            game.board.io_name_data, sizeof(game.board.io_name_data));
}

JNIEXPORT jbyteArray JNICALL
Java_com_kerry_zzt_ZztNative_font8x14(JNIEnv *env, jclass clazz) {
    (void) clazz;
    jbyteArray result = (*env)->NewByteArray(env, FONT_BYTES);
    if (result != NULL) (*env)->SetByteArrayRegion(env, result, 0, FONT_BYTES, (const jbyte *) res_8x14_bin);
    return result;
}

JNIEXPORT void JNICALL
Java_com_kerry_zzt_ZztNative_renderAudio(JNIEnv *env, jclass clazz, jbyteArray target) {
    (void) clazz;
    if (target == NULL) return;
    jsize length = (*env)->GetArrayLength(env, target);
    jbyte *samples = (*env)->GetByteArrayElements(env, target, NULL);
    if (samples == NULL) return;
    pthread_mutex_lock(&sound_mutex);
    if (ready) {
        zoo_sound_pcm_generate(&sound_pcm, (uint8_t *) samples, (size_t) length);
    } else {
        memset(samples, 0x80, (size_t) length);
    }
    pthread_mutex_unlock(&sound_mutex);
    (*env)->ReleaseByteArrayElements(env, target, samples, 0);
}
