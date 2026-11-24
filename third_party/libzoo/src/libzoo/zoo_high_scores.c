#include <string.h>

#include "zoo_high_scores.h"

#define HIGH_SCORE_NAME_LENGTH_OFFSET 0
#define HIGH_SCORE_NAME_OFFSET 1
#define HIGH_SCORE_SCORE_OFFSET 51

static uint8_t *zoo_high_score_entry(zoo_high_score_table *table, int16_t index) {
    return table->bytes + (size_t) index * ZOO_HIGH_SCORE_ENTRY_BYTES;
}

static const uint8_t *zoo_high_score_entry_const(const zoo_high_score_table *table,
        int16_t index) {
    return table->bytes + (size_t) index * ZOO_HIGH_SCORE_ENTRY_BYTES;
}

static int16_t zoo_high_score_read_i16(const uint8_t *bytes) {
    uint16_t raw = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8);
    int32_t value = (raw & 0x8000u) ? (int32_t) raw - 0x10000 : (int32_t) raw;
    return (int16_t) value;
}

static void zoo_high_score_write_i16(uint8_t *bytes, int16_t value) {
    uint16_t raw = (uint16_t) value;
    bytes[0] = (uint8_t) raw;
    bytes[1] = (uint8_t) (raw >> 8);
}

static void zoo_high_score_assign_name(uint8_t *entry, const uint8_t *name,
        size_t length) {
    if (length > ZOO_HIGH_SCORE_NAME_BYTES) length = ZOO_HIGH_SCORE_NAME_BYTES;
    entry[HIGH_SCORE_NAME_LENGTH_OFFSET] = (uint8_t) length;
    if (length > 0) memcpy(entry + HIGH_SCORE_NAME_OFFSET, name, length);
}

void zoo_high_score_table_init(zoo_high_score_table *table) {
    if (table == NULL) return;
    memset(table->bytes, 0, sizeof(table->bytes));
    for (int16_t i = 0; i < ZOO_HIGH_SCORE_COUNT; i++) {
        zoo_high_score_write_i16(zoo_high_score_entry(table, i) + HIGH_SCORE_SCORE_OFFSET, -1);
    }
}

void zoo_high_score_table_reset_after_io_error(zoo_high_score_table *table) {
    if (table == NULL) return;
    for (int16_t i = 0; i < ZOO_HIGH_SCORE_COUNT; i++) {
        uint8_t *entry = zoo_high_score_entry(table, i);
        entry[HIGH_SCORE_NAME_LENGTH_OFFSET] = 0;
        zoo_high_score_write_i16(entry + HIGH_SCORE_SCORE_OFFSET, -1);
        /* Pascal leaves the 50 name payload bytes untouched on reset. */
    }
}

int zoo_high_score_table_load(zoo_high_score_table *table, const uint8_t *record,
        size_t length, bool read_succeeded) {
    if (table == NULL) return ZOO_ERROR_INVAL;
    if (!read_succeeded || record == NULL || length < ZOO_HIGH_SCORE_TABLE_BYTES) {
        zoo_high_score_table_reset_after_io_error(table);
        return ZOO_ERROR_INVAL;
    }
    /* Pascal reads exactly one typed THighScoreList record and ignores trailing bytes. */
    memcpy(table->bytes, record, ZOO_HIGH_SCORE_TABLE_BYTES);
    return 0;
}

int zoo_high_score_table_save(const zoo_high_score_table *table, uint8_t *output,
        size_t capacity) {
    if (table == NULL || output == NULL || capacity < ZOO_HIGH_SCORE_TABLE_BYTES) {
        return ZOO_ERROR_INVAL;
    }
    memcpy(output, table->bytes, ZOO_HIGH_SCORE_TABLE_BYTES);
    return 0;
}

int16_t zoo_high_score_entry_score(const zoo_high_score_table *table, int16_t index) {
    if (table == NULL || index < 0 || index >= ZOO_HIGH_SCORE_COUNT) {
        return ZOO_ERROR_INVAL;
    }
    return zoo_high_score_read_i16(zoo_high_score_entry_const(table, index)
            + HIGH_SCORE_SCORE_OFFSET);
}

uint8_t zoo_high_score_entry_name_length(const zoo_high_score_table *table,
        int16_t index) {
    if (table == NULL || index < 0 || index >= ZOO_HIGH_SCORE_COUNT) return 0;
    uint8_t length = zoo_high_score_entry_const(table, index)[HIGH_SCORE_NAME_LENGTH_OFFSET];
    return length > ZOO_HIGH_SCORE_NAME_BYTES ? ZOO_HIGH_SCORE_NAME_BYTES : length;
}

size_t zoo_high_score_entry_copy_name(const zoo_high_score_table *table,
        int16_t index, uint8_t *output, size_t capacity) {
    if (table == NULL || index < 0 || index >= ZOO_HIGH_SCORE_COUNT) return 0;
    size_t length = zoo_high_score_entry_name_length(table, index);
    if (length > capacity) length = capacity;
    if (length > 0 && output == NULL) return 0;
    if (length > 0) {
        memcpy(output, zoo_high_score_entry_const(table, index) + HIGH_SCORE_NAME_OFFSET, length);
    }
    return length;
}

int16_t zoo_high_score_insertion_position(const zoo_high_score_table *table,
        int16_t score) {
    if (table == NULL) return -1;
    int16_t position = 0;
    while (position < ZOO_HIGH_SCORE_COUNT
            && score < zoo_high_score_entry_score(table, position)) {
        position++;
    }
    return score > 0 && position < ZOO_HIGH_SCORE_COUNT ? position : -1;
}

int zoo_high_score_insert_at(zoo_high_score_table *table, int16_t position,
        int16_t score, const uint8_t *name, size_t name_length) {
    if (table == NULL || position < 0 || position >= ZOO_HIGH_SCORE_COUNT
            || (name_length > 0 && name == NULL)) {
        return ZOO_ERROR_INVAL;
    }
    for (int16_t i = ZOO_HIGH_SCORE_COUNT - 1; i > position; i--) {
        memcpy(zoo_high_score_entry(table, i), zoo_high_score_entry(table, (int16_t) (i - 1)),
                ZOO_HIGH_SCORE_ENTRY_BYTES);
    }

    uint8_t *entry = zoo_high_score_entry(table, position);
    zoo_high_score_write_i16(entry + HIGH_SCORE_SCORE_OFFSET, score);
    static const uint8_t placeholder[] = "-- You! --";
    zoo_high_score_assign_name(entry, placeholder, sizeof(placeholder) - 1);
    zoo_high_score_assign_name(entry, name, name_length);
    return 0;
}
