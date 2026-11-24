#include <stddef.h>
#include <stdint.h>

#include "zoo_high_scores.h"

static zoo_high_score_table table;
static uint8_t input[4096];
static uint8_t name[256];
static uint8_t output[ZOO_HIGH_SCORE_TABLE_BYTES];

void *memcpy(void *destination, const void *source, size_t length) {
    uint8_t *to = (uint8_t *) destination;
    const uint8_t *from = (const uint8_t *) source;
    while (length-- > 0) *to++ = *from++;
    return destination;
}

void *memset(void *destination, int value, size_t length) {
    uint8_t *to = (uint8_t *) destination;
    while (length-- > 0) *to++ = (uint8_t) value;
    return destination;
}

uint32_t hs_table_ptr(void) { return (uint32_t) (uintptr_t) table.bytes; }
uint32_t hs_input_ptr(void) { return (uint32_t) (uintptr_t) input; }
uint32_t hs_name_ptr(void) { return (uint32_t) (uintptr_t) name; }
uint32_t hs_output_ptr(void) { return (uint32_t) (uintptr_t) output; }

int hs_init(void) {
    zoo_high_score_table_init(&table);
    return 0;
}

int hs_load(uint32_t length, int read_succeeded) {
    if (length > sizeof(input)) return ZOO_ERROR_INVAL;
    return zoo_high_score_table_load(&table, input, length, read_succeeded != 0);
}

int hs_save(void) {
    return zoo_high_score_table_save(&table, output, sizeof(output));
}

int hs_insertion_position(int score) {
    if (score < INT16_MIN || score > INT16_MAX) return -1;
    return zoo_high_score_insertion_position(&table, (int16_t) score);
}

int hs_insert(int position, int score, uint32_t name_length) {
    if (position < INT16_MIN || position > INT16_MAX
            || score < INT16_MIN || score > INT16_MAX
            || name_length > sizeof(name)) return ZOO_ERROR_INVAL;
    return zoo_high_score_insert_at(&table, (int16_t) position, (int16_t) score,
            name, name_length);
}

int hs_score(int index) {
    if (index < INT16_MIN || index > INT16_MAX) return ZOO_ERROR_INVAL;
    return zoo_high_score_entry_score(&table, (int16_t) index);
}
