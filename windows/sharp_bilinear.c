#include "sharp_bilinear.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	int first;
	int second;
	uint32_t fraction;
} sample_axis;

bool zzt_fit_4_3_viewport(int available_width, int available_height,
	int *x_out, int *y_out, int *width_out, int *height_out) {
	int width;
	int height;
	if (available_width <= 0 || available_height <= 0 || x_out == NULL || y_out == NULL
		|| width_out == NULL || height_out == NULL) return false;
	if ((int64_t) available_width * 3 <= (int64_t) available_height * 4) {
		width = available_width;
		height = (int) ((int64_t) available_width * 3 / 4);
	} else {
		height = available_height;
		width = (int) ((int64_t) available_height * 4 / 3);
	}
	if (width <= 0 || height <= 0) return false;
	*x_out = (available_width - width) / 2;
	*y_out = (available_height - height) / 2;
	*width_out = width;
	*height_out = height;
	return true;
}

static bool sample_axis_init(int coordinate, int source_size, int destination_size,
	int integer_scale, sample_axis *sample) {
	int64_t enlarged_size = (int64_t) source_size * integer_scale;
	int64_t fixed_coordinate;
	int64_t maximum;
	int64_t numerator;
	int64_t quotient;
	int64_t remainder;
	int first;
	int second;
	if (enlarged_size <= 0 || enlarged_size > INT_MAX) return false;

	/* Pixel-center mapping keeps identity and integer scales exact. */
	numerator = (((int64_t) coordinate * 2) + 1) * enlarged_size;
	quotient = numerator / destination_size;
	remainder = numerator % destination_size;
	fixed_coordinate = quotient * 32768
		+ (remainder * 32768) / destination_size - 32768;
	maximum = (enlarged_size - 1) * 65536;
	if (fixed_coordinate <= 0) {
		first = 0;
		second = 0;
		sample->fraction = 0;
	} else if (fixed_coordinate >= maximum) {
		first = (int) enlarged_size - 1;
		second = first;
		sample->fraction = 0;
	} else {
		first = (int) (fixed_coordinate >> 16);
		second = first + 1;
		sample->fraction = (uint32_t) (fixed_coordinate & 0xffff);
	}
	/* Cache the corresponding original source pixels. The intermediate integer
	   enlargement is virtual, so these divisions otherwise happen four times
	   for every destination pixel during the bilinear pass. */
	sample->first = first / integer_scale;
	sample->second = second / integer_scale;
	return true;
}

static uint32_t blend_channel(uint32_t p00, uint32_t p10, uint32_t p01, uint32_t p11,
	uint64_t w00, uint64_t w10, uint64_t w01, uint64_t w11, unsigned shift) {
	uint64_t value =
		(uint64_t) ((p00 >> shift) & 0xff) * w00
		+ (uint64_t) ((p10 >> shift) & 0xff) * w10
		+ (uint64_t) ((p01 >> shift) & 0xff) * w01
		+ (uint64_t) ((p11 >> shift) & 0xff) * w11;
	return (uint32_t) ((value + 0x80000000ULL) >> 32);
}

bool zzt_sharp_bilinear_scale(const uint32_t *source, int source_width, int source_height,
	uint32_t *destination, int destination_width, int destination_height,
	int *integer_scale_x_out, int *integer_scale_y_out) {
	int integer_scale_x;
	int integer_scale_y;
	sample_axis *x_samples;
	sample_axis *y_samples;
	int y;
	if (source == NULL || destination == NULL || source_width <= 0 || source_height <= 0
		|| destination_width <= 0 || destination_height <= 0) return false;
	if ((size_t) source_width > SIZE_MAX / (size_t) source_height / sizeof(uint32_t)
		|| (size_t) destination_width > SIZE_MAX / (size_t) destination_height / sizeof(uint32_t)
		|| (size_t) destination_width > SIZE_MAX / sizeof(sample_axis)
		|| (size_t) destination_height > SIZE_MAX / sizeof(sample_axis)) return false;

	/* Replicate whole source pixels independently on each axis as far as that
	   axis fits. This preserves an integer pixel grid before the final bilinear
	   pass, including when DOS pixel-aspect correction makes the two residual
	   scales different (for example 640x350 into a 960x720 4:3 viewport). */
	integer_scale_x = destination_width / source_width;
	integer_scale_y = destination_height / source_height;
	if (integer_scale_x < 1) integer_scale_x = 1;
	if (integer_scale_y < 1) integer_scale_y = 1;
	if ((int64_t) source_width * integer_scale_x > INT_MAX
		|| (int64_t) source_height * integer_scale_y > INT_MAX) return false;
	if (integer_scale_x_out != NULL) *integer_scale_x_out = integer_scale_x;
	if (integer_scale_y_out != NULL) *integer_scale_y_out = integer_scale_y;
	x_samples = (sample_axis *) malloc((size_t) destination_width * sizeof(sample_axis));
	y_samples = (sample_axis *) malloc((size_t) destination_height * sizeof(sample_axis));
	if (x_samples == NULL || y_samples == NULL) {
		free(x_samples);
		free(y_samples);
		return false;
	}
	for (y = 0; y < destination_height; y++) {
		if (!sample_axis_init(y, source_height, destination_height, integer_scale_y, &y_samples[y])) {
			free(x_samples);
			free(y_samples);
			return false;
		}
	}
	for (y = 0; y < destination_width; y++) {
		if (!sample_axis_init(y, source_width, destination_width, integer_scale_x, &x_samples[y])) {
			free(x_samples);
			free(y_samples);
			return false;
		}
	}

	for (y = 0; y < destination_height; y++) {
		sample_axis sy = y_samples[y];
		int x;
		for (x = 0; x < destination_width; x++) {
			sample_axis sx = x_samples[x];
			uint64_t fx, fy, ix, iy;
			uint64_t w00, w10, w01, w11;
			uint32_t p00, p10, p01, p11;
			uint32_t alpha, red, green, blue;
			fx = sx.fraction;
			fy = sy.fraction;
			ix = 65536 - fx;
			iy = 65536 - fy;
			w00 = ix * iy;
			w10 = fx * iy;
			w01 = ix * fy;
			w11 = fx * fy;
			p00 = source[(size_t) sy.first * (size_t) source_width + (size_t) sx.first];
			p10 = source[(size_t) sy.first * (size_t) source_width + (size_t) sx.second];
			p01 = source[(size_t) sy.second * (size_t) source_width + (size_t) sx.first];
			p11 = source[(size_t) sy.second * (size_t) source_width + (size_t) sx.second];
			alpha = blend_channel(p00, p10, p01, p11, w00, w10, w01, w11, 24);
			red = blend_channel(p00, p10, p01, p11, w00, w10, w01, w11, 16);
			green = blend_channel(p00, p10, p01, p11, w00, w10, w01, w11, 8);
			blue = blend_channel(p00, p10, p01, p11, w00, w10, w01, w11, 0);
			destination[(size_t) y * (size_t) destination_width + (size_t) x]
				= (alpha << 24) | (red << 16) | (green << 8) | blue;
		}
	}
	free(x_samples);
	free(y_samples);
	return true;
}
