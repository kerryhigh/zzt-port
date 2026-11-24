#ifndef ZOO_EDITOR_H
#define ZOO_EDITOR_H

#include <stddef.h>
#include <stdint.h>
#include "zoo.h"

/*
 * Apply the platform-specific editor placement operation to one cell.
 * Return zero when applied, a positive value when the operation refuses the
 * cell, or a negative ZOO_ERROR_* value on failure.
 */
typedef int (*zoo_editor_flood_fill_apply_fn)(zoo_state *state, int16_t x,
        int16_t y, void *context);

/*
 * Fill the connected component containing (x, y), preserving the original
 * ZZT editor's 256-entry byte-indexed queue and traversal order.
 * The return value is the number of applied tile changes, or ZOO_ERROR_*.
 */
int zoo_editor_flood_fill(zoo_state *state, int16_t x, int16_t y,
        zoo_editor_flood_fill_apply_fn apply, void *context);

#endif
