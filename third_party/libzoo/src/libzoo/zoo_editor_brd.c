#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libzoo/zoo_internal.h"
#include "zoo_editor_brd.h"

typedef struct {
    const uint8_t *data;
    size_t length;
    size_t position;
} editor_brd_reader;

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t position;
} editor_brd_writer;

static bool editor_brd_read(editor_brd_reader *reader, void *target, size_t count) {
    if (count > reader->length - reader->position) return false;
    if (count > 0) memcpy(target, reader->data + reader->position, count);
    reader->position += count;
    return true;
}

static uint16_t editor_brd_u16(const uint8_t *data) {
    return (uint16_t) (data[0] | ((uint16_t) data[1] << 8));
}

static int16_t zoo_editor_brd_i16(const uint8_t *data) {
    return (int16_t) editor_brd_u16(data);
}

static bool editor_brd_write(editor_brd_writer *writer, const void *data, size_t count) {
    if (count > writer->capacity - writer->position) return false;
    if (count > 0) memcpy(writer->data + writer->position, data, count);
    writer->position += count;
    return true;
}

static bool editor_brd_write_u8(editor_brd_writer *writer, uint8_t value) {
    return editor_brd_write(writer, &value, 1);
}

static bool editor_brd_write_u16(editor_brd_writer *writer, int16_t value) {
    uint8_t bytes[2] = {(uint8_t) value, (uint8_t) ((uint16_t) value >> 8)};
    return editor_brd_write(writer, bytes, sizeof(bytes));
}

static size_t editor_visible_string_length(const char *value, size_t capacity) {
    size_t length = 0;
    while (length < capacity && value[length] != '\0') length++;
    return length;
}

void zoo_editor_brd_free(zoo_board *board) {
    for (int i = 0; i <= board->stat_count; i++) {
        char *data = board->stats[i].data;
        if (data == NULL) continue;
        bool shared_before = false;
        for (int j = 0; j < i; j++) {
            if (board->stats[j].data == data) {
                shared_before = true;
                break;
            }
        }
        if (!shared_before) free(data);
    }
}

int zoo_editor_brd_parse(const uint8_t *bytes, size_t length, zoo_board *board) {
    editor_brd_reader reader = {bytes, length, 0};
    uint8_t name_raw[51];
    if (!editor_brd_read(&reader, name_raw, sizeof(name_raw))) return ZOO_ERROR_INVAL;
    memset(board, 0, sizeof(*board));
    board->io_name_valid = true;
    board->io_name_length = name_raw[0];
    memcpy(board->io_name_data, name_raw + 1, sizeof(board->io_name_data));
    size_t name_length = name_raw[0] > 50 ? 50 : name_raw[0];
    memcpy(board->name, name_raw + 1, name_length);
    board->name[name_length] = '\0';

    zoo_tile edge = {ZOO_E_BOARD_EDGE, 0};
    for (int x = 0; x <= ZOO_BOARD_WIDTH + 1; x++) {
        board->tiles[x][0] = edge;
        board->tiles[x][ZOO_BOARD_HEIGHT + 1] = edge;
    }
    for (int y = 0; y <= ZOO_BOARD_HEIGHT + 1; y++) {
        board->tiles[0][y] = edge;
        board->tiles[ZOO_BOARD_WIDTH + 1][y] = edge;
    }

    int tile_index = 0;
    uint8_t rle_remaining = 0;
    zoo_tile rle_tile = {0, 0};
    while (tile_index < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT) {
        if (rle_remaining == 0) {
            uint8_t count;
            if (!editor_brd_read(&reader, &count, 1)
                    || !editor_brd_read(&reader, &rle_tile, sizeof(rle_tile))) {
                return ZOO_ERROR_INVAL;
            }
            rle_remaining = count;
        }
        int x = (tile_index % ZOO_BOARD_WIDTH) + 1;
        int y = (tile_index / ZOO_BOARD_WIDTH) + 1;
        board->tiles[x][y] = rle_tile;
        tile_index++;
        // Pascal decrements a Byte; zero therefore wraps to 255.
        rle_remaining = (uint8_t) (rle_remaining - 1u);
    }

    uint8_t info_raw[86];
    if (!editor_brd_read(&reader, info_raw, sizeof(info_raw))) return ZOO_ERROR_INVAL;
    zoo_board_info *info = &board->info;
    info->max_shots = info_raw[0];
    info->io_is_dark_byte = info_raw[1];
    info->is_dark = info_raw[1] != 0;
    memcpy(info->neighbor_boards, info_raw + 2, 4);
    info->io_reenter_when_zapped_byte = info_raw[6];
    info->reenter_when_zapped = info_raw[6] != 0;
    info->io_boolean_bytes_valid = true;
    info->io_message_length = info_raw[7];
    memcpy(info->io_message_data, info_raw + 8, sizeof(info->io_message_data));
    size_t message_length = info_raw[7] > ZOO_LEN_MESSAGE ? ZOO_LEN_MESSAGE : info_raw[7];
    memcpy(info->message, info_raw + 8, message_length);
    info->message[message_length] = '\0';
    info->io_message_valid = true;
    info->start_player_x = info_raw[66];
    info->start_player_y = info_raw[67];
    info->time_limit_sec = zoo_editor_brd_i16(info_raw + 68);
    memcpy(info->io_reserved, info_raw + 70, sizeof(info->io_reserved));

    uint8_t count_raw[2];
    if (!editor_brd_read(&reader, count_raw, sizeof(count_raw))) return ZOO_ERROR_INVAL;
    int16_t stat_count = zoo_editor_brd_i16(count_raw);
    if (stat_count < 0 || stat_count > ZOO_MAX_STAT) return ZOO_ERROR_INVAL;
    board->stat_count = stat_count;
    for (int i = 0; i <= stat_count; i++) {
        uint8_t raw[33];
        if (!editor_brd_read(&reader, raw, sizeof(raw))) return ZOO_ERROR_INVAL;
        zoo_stat *stat = &board->stats[i];
        stat->x = raw[0];
        stat->y = raw[1];
        stat->step_x = zoo_editor_brd_i16(raw + 2);
        stat->step_y = zoo_editor_brd_i16(raw + 4);
        stat->cycle = zoo_editor_brd_i16(raw + 6);
        stat->p1 = raw[8];
        stat->p2 = raw[9];
        stat->p3 = raw[10];
        stat->follower = zoo_editor_brd_i16(raw + 11);
        stat->leader = zoo_editor_brd_i16(raw + 13);
        stat->under.element = raw[15];
        stat->under.color = raw[16];
        memcpy(stat->io_data_pointer, raw + 17, sizeof(stat->io_data_pointer));
        stat->data_pos = zoo_editor_brd_i16(raw + 21);
        stat->data_len = zoo_editor_brd_i16(raw + 23);
        memcpy(stat->io_reserved, raw + 25, sizeof(stat->io_reserved));
        if (stat->data_len > 0) {
            if ((size_t) stat->data_len > reader.length - reader.position) return ZOO_ERROR_INVAL;
            stat->data = (char *) malloc((size_t) stat->data_len);
            if (stat->data == NULL) return ZOO_ERROR_NOMEM;
            if (!editor_brd_read(&reader, stat->data, (size_t) stat->data_len)) return ZOO_ERROR_INVAL;
        } else if (stat->data_len < 0) {
            int reference = -((int) stat->data_len);
            if (reference >= i || reference < 0) return ZOO_ERROR_INVAL;
            stat->data = board->stats[reference].data;
            stat->data_len = board->stats[reference].data_len;
        }
    }
    return 0;
}

int zoo_editor_brd_import_record(const uint8_t *bytes, size_t length, zoo_board *board) {
    if (bytes == NULL || board == NULL || length < 2) return ZOO_ERROR_INVAL;
    uint16_t board_length = editor_brd_u16(bytes);
    if ((size_t) board_length > length - 2) return ZOO_ERROR_INVAL;
    return zoo_editor_brd_parse(bytes + 2, board_length, board);
}

static bool editor_write_pascal_string(editor_brd_writer *writer, const char *value,
        size_t max_length, bool raw_valid, uint8_t raw_length, const uint8_t *raw_data) {
    size_t current_length = editor_visible_string_length(value, max_length);
    size_t visible_raw_length = raw_length > max_length ? max_length : raw_length;
    // A Pascal short string can contain NULs while its length byte still includes later bytes.
    size_t raw_c_string_length = editor_visible_string_length((const char *) raw_data,
            visible_raw_length);
    if (raw_valid && current_length == raw_c_string_length
            && memcmp(value, raw_data, raw_c_string_length) == 0) {
        return editor_brd_write_u8(writer, raw_length)
                && editor_brd_write(writer, raw_data, max_length);
    }
    if (current_length > max_length) current_length = max_length;
    if (!editor_brd_write_u8(writer, (uint8_t) current_length)
            || !editor_brd_write(writer, value, current_length)) return false;
    size_t padding_length = max_length - current_length;
    if (padding_length == 0) return true;
    if (raw_valid) return editor_brd_write(writer, raw_data + current_length, padding_length);
    static const uint8_t zeros[ZOO_LEN_MESSAGE] = {0};
    return editor_brd_write(writer, zeros, padding_length);
}

int zoo_editor_brd_serialize(const zoo_board *board, uint8_t *output, size_t capacity,
                                size_t *output_length) {
    editor_brd_writer writer = {output, capacity, 0};
    if (!editor_write_pascal_string(&writer, board->name, 50, board->io_name_valid,
            board->io_name_length, board->io_name_data)) return ZOO_ERROR_INVAL;

    int tile_index = 0;
    zoo_tile run_tile = board->tiles[1][1];
    uint8_t run_count = 1;
    for (tile_index = 1; tile_index < ZOO_BOARD_WIDTH * ZOO_BOARD_HEIGHT; tile_index++) {
        int x = (tile_index % ZOO_BOARD_WIDTH) + 1;
        int y = (tile_index / ZOO_BOARD_WIDTH) + 1;
        zoo_tile tile = board->tiles[x][y];
        if (tile.element == run_tile.element && tile.color == run_tile.color && run_count < 255) {
            run_count++;
        } else {
            if (!editor_brd_write_u8(&writer, run_count)
                    || !editor_brd_write(&writer, &run_tile, sizeof(run_tile))) return ZOO_ERROR_INVAL;
            run_tile = tile;
            run_count = 1;
        }
    }
    if (!editor_brd_write_u8(&writer, run_count)
            || !editor_brd_write(&writer, &run_tile, sizeof(run_tile))) return ZOO_ERROR_INVAL;

    const zoo_board_info *info = &board->info;
    uint8_t dark = info->io_boolean_bytes_valid
            && ((info->io_is_dark_byte != 0) == info->is_dark)
            ? info->io_is_dark_byte : (info->is_dark ? 1 : 0);
    uint8_t reenter = info->io_boolean_bytes_valid
            && ((info->io_reenter_when_zapped_byte != 0) == info->reenter_when_zapped)
            ? info->io_reenter_when_zapped_byte : (info->reenter_when_zapped ? 1 : 0);
    if (!editor_brd_write_u8(&writer, info->max_shots)
            || !editor_brd_write_u8(&writer, dark)
            || !editor_brd_write(&writer, info->neighbor_boards, sizeof(info->neighbor_boards))
            || !editor_brd_write_u8(&writer, reenter)
            || !editor_write_pascal_string(&writer, info->message, ZOO_LEN_MESSAGE,
                    info->io_message_valid, info->io_message_length, info->io_message_data)
            || !editor_brd_write_u8(&writer, info->start_player_x)
            || !editor_brd_write_u8(&writer, info->start_player_y)
            || !editor_brd_write_u16(&writer, info->time_limit_sec)
            || !editor_brd_write(&writer, info->io_reserved, sizeof(info->io_reserved))
            || !editor_brd_write_u16(&writer, board->stat_count)) return ZOO_ERROR_INVAL;

    for (int i = 0; i <= board->stat_count; i++) {
        const zoo_stat *stat = &board->stats[i];
        int16_t wire_data_len = stat->data_len;
        if (wire_data_len > 0) {
            for (int j = 1; j < i; j++) {
                if (board->stats[j].data == stat->data) wire_data_len = (int16_t) -j;
            }
        }
        if (!editor_brd_write_u8(&writer, stat->x)
                || !editor_brd_write_u8(&writer, stat->y)
                || !editor_brd_write_u16(&writer, stat->step_x)
                || !editor_brd_write_u16(&writer, stat->step_y)
                || !editor_brd_write_u16(&writer, stat->cycle)
                || !editor_brd_write_u8(&writer, stat->p1)
                || !editor_brd_write_u8(&writer, stat->p2)
                || !editor_brd_write_u8(&writer, stat->p3)
                || !editor_brd_write_u16(&writer, stat->follower)
                || !editor_brd_write_u16(&writer, stat->leader)
                || !editor_brd_write_u8(&writer, stat->under.element)
                || !editor_brd_write_u8(&writer, stat->under.color)
                || !editor_brd_write(&writer, stat->io_data_pointer, sizeof(stat->io_data_pointer))
                || !editor_brd_write_u16(&writer, stat->data_pos)
                || !editor_brd_write_u16(&writer, wire_data_len)
                || !editor_brd_write(&writer, stat->io_reserved, sizeof(stat->io_reserved))) {
            return ZOO_ERROR_INVAL;
        }
        if (wire_data_len > 0) {
            if (stat->data == NULL || !editor_brd_write(&writer, stat->data, (size_t) wire_data_len)) {
                return ZOO_ERROR_INVAL;
            }
        }
    }
    *output_length = writer.position;
    return 0;
}

