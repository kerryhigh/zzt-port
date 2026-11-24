#include <stdint.h>

#include "zoo_editor.h"

int zoo_editor_flood_fill(zoo_state *state, int16_t x, int16_t y,
        zoo_editor_flood_fill_apply_fn apply, void *context) {
    if (state == NULL || apply == NULL
            || x < 1 || x > ZOO_BOARD_WIDTH
            || y < 1 || y > ZOO_BOARD_HEIGHT) {
        return ZOO_ERROR_INVAL;
    }

    static const int16_t dx[4] = {0, 0, -1, 1};
    static const int16_t dy[4] = {-1, 1, 0, 0};
    /* Pascal uses 256 integer positions and byte queue counters. */
    uint8_t queue_x[256] = {0};
    uint8_t queue_y[256] = {0};
    uint8_t to_fill = 1;
    uint8_t filled = 0;
    int changed = 0;
    zoo_tile source = state->board.tiles[x][y];

    while (to_fill != filled) {
        int16_t cell_x = x;
        int16_t cell_y = y;
        zoo_tile before = state->board.tiles[cell_x][cell_y];
        int result = apply(state, cell_x, cell_y, context);
        if (result < 0) return result;

        zoo_tile after = state->board.tiles[cell_x][cell_y];
        if (result == 0 && (before.element != after.element
                || before.color != after.color)) {
            changed++;
            for (int i = 0; i < 4; i++) {
                int16_t next_x = cell_x + dx[i];
                int16_t next_y = cell_y + dy[i];
                if (next_x < 0 || next_x > ZOO_BOARD_WIDTH + 1
                        || next_y < 0 || next_y > ZOO_BOARD_HEIGHT + 1) {
                    continue;
                }
                zoo_tile candidate = state->board.tiles[next_x][next_y];
                if (candidate.element == source.element
                        && (source.element == ZOO_E_EMPTY
                                || candidate.color == source.color)) {
                    queue_x[to_fill] = (uint8_t) next_x;
                    queue_y[to_fill] = (uint8_t) next_y;
                    to_fill = (uint8_t) (to_fill + 1u);
                }
            }
        }

        filled = (uint8_t) (filled + 1u);
        x = queue_x[filled];
        y = queue_y[filled];
    }

    return changed;
}
