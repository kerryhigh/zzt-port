#ifndef ZZT_SHARP_BILINEAR_H
#define ZZT_SHARP_BILINEAR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The largest integer nearest-neighbor enlargement that fits each destination
 * axis, followed by bilinear resizing for the remaining fractional enlargement
 * and any display pixel-aspect correction. The intermediate is sampled
 * virtually to avoid allocating another, potentially very large framebuffer.
 */
bool zzt_sharp_bilinear_scale(const uint32_t *source, int source_width, int source_height,
	uint32_t *destination, int destination_width, int destination_height,
	int *integer_scale_x_out, int *integer_scale_y_out);

/* Center the largest integer-sized 4:3 viewport that fits an available area. */
bool zzt_fit_4_3_viewport(int available_width, int available_height,
	int *x_out, int *y_out, int *width_out, int *height_out);

#endif
