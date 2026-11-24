#ifndef ZOO_HIGH_SCORES_H
#define ZOO_HIGH_SCORES_H

#include <stddef.h>
#include <stdint.h>
#include "zoo.h"

#define ZOO_HIGH_SCORE_COUNT 30
#define ZOO_HIGH_SCORE_NAME_BYTES 50
#define ZOO_HIGH_SCORE_ENTRY_BYTES 53
#define ZOO_HIGH_SCORE_TABLE_BYTES \
    (ZOO_HIGH_SCORE_COUNT * ZOO_HIGH_SCORE_ENTRY_BYTES)

typedef struct {
    uint8_t bytes[ZOO_HIGH_SCORE_TABLE_BYTES];
} zoo_high_score_table;

void zoo_high_score_table_init(zoo_high_score_table *table);
void zoo_high_score_table_reset_after_io_error(zoo_high_score_table *table);
int zoo_high_score_table_load(zoo_high_score_table *table, const uint8_t *record,
        size_t length, bool read_succeeded);
int zoo_high_score_table_save(const zoo_high_score_table *table, uint8_t *output,
        size_t capacity);

int16_t zoo_high_score_entry_score(const zoo_high_score_table *table, int16_t index);
uint8_t zoo_high_score_entry_name_length(const zoo_high_score_table *table,
        int16_t index);
size_t zoo_high_score_entry_copy_name(const zoo_high_score_table *table,
        int16_t index, uint8_t *output, size_t capacity);
int16_t zoo_high_score_insertion_position(const zoo_high_score_table *table,
        int16_t score);
int zoo_high_score_insert_at(zoo_high_score_table *table, int16_t position,
        int16_t score, const uint8_t *name, size_t name_length);

#endif
