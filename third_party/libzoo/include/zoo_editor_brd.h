#ifndef ZOO_EDITOR_BRD_H
#define ZOO_EDITOR_BRD_H

#include <stddef.h>
#include <stdint.h>
#include "zoo.h"

void zoo_editor_brd_free(zoo_board *board);
int zoo_editor_brd_import_record(const uint8_t *bytes, size_t length, zoo_board *board);
typedef enum {
    ZOO_EDITOR_BRD_TRANSFER_IMPORTED = 0,
    ZOO_EDITOR_BRD_TRANSFER_REOPENED_PREVIOUS = 1,
    ZOO_EDITOR_BRD_TRANSFER_RESET_AFTER_SHORT_PAYLOAD = 2
} zoo_editor_brd_transfer_outcome;
int zoo_editor_brd_transfer(zoo_state *state, const uint8_t *bytes, size_t length,
        zoo_editor_brd_transfer_outcome *outcome);
int zoo_editor_brd_parse(const uint8_t *bytes, size_t length, zoo_board *board);
int zoo_editor_brd_serialize(const zoo_board *board, uint8_t *output,
                         size_t capacity, size_t *output_length);

#endif
