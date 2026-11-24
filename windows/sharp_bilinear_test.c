#include "sharp_bilinear.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

enum {
	NATIVE_WIDTH = 640,
	NATIVE_HEIGHT = 350,
	CRT_WIDTH = 640,
	CRT_HEIGHT = 480,
	WINDOW_WIDTH = 960,
	WINDOW_HEIGHT = 720,
	FULLSCREEN_WIDTH = 1440,
	FULLSCREEN_HEIGHT = 1080
};

static uint32_t native_frame[NATIVE_WIDTH * NATIVE_HEIGHT];
static uint32_t crt_frame[CRT_WIDTH * CRT_HEIGHT];
static uint32_t window_frame[WINDOW_WIDTH * WINDOW_HEIGHT];
static uint32_t fullscreen_frame[FULLSCREEN_WIDTH * FULLSCREEN_HEIGHT];

static void reference_sample_axis(double coordinate, int source_size, int destination_size,
	int *first, int *second, double *fraction) {
	double position = ((coordinate + 0.5) * source_size / destination_size) - 0.5;
	if (position <= 0.0) {
		*first = 0;
		*second = 0;
		*fraction = 0.0;
	} else if (position >= source_size - 1) {
		*first = source_size - 1;
		*second = *first;
		*fraction = 0.0;
	} else {
		*first = (int) position;
		*second = *first + 1;
		*fraction = position - *first;
	}
}

static uint32_t reference_blend(uint32_t p00, uint32_t p10, uint32_t p01, uint32_t p11,
	double fx, double fy) {
	uint32_t result = 0;
	unsigned shift;
	for (shift = 0; shift <= 24; shift += 8) {
		double top = ((p00 >> shift) & 0xff) * (1.0 - fx) + ((p10 >> shift) & 0xff) * fx;
		double bottom = ((p01 >> shift) & 0xff) * (1.0 - fx) + ((p11 >> shift) & 0xff) * fx;
		unsigned channel = (unsigned) (top * (1.0 - fy) + bottom * fy + 0.5);
		if (channel > 255) channel = 255;
		result |= (uint32_t) channel << shift;
	}
	return result;
}

/* Build the requested integer nearest-neighbor intermediate explicitly, then
   bilinearly resize it. This slower reference proves that the production
   scaler's virtual intermediate has the same image without a second framebuffer. */
static int materialized_nearest_then_bilinear(const uint32_t *source,
	int source_width, int source_height, int integer_scale_x, int integer_scale_y,
	uint32_t *destination, int destination_width, int destination_height) {
	int enlarged_width = source_width * integer_scale_x;
	int enlarged_height = source_height * integer_scale_y;
	size_t pixel_count = (size_t) enlarged_width * (size_t) enlarged_height;
	uint32_t *enlarged = (uint32_t *) malloc(pixel_count * sizeof(uint32_t));
	int y;
	if (enlarged == NULL) return 0;
	for (y = 0; y < enlarged_height; y++) {
		int x;
		for (x = 0; x < enlarged_width; x++) {
			enlarged[(size_t) y * enlarged_width + x]
				= source[(size_t) (y / integer_scale_y) * source_width + x / integer_scale_x];
		}
	}
	for (y = 0; y < destination_height; y++) {
		int y0, y1, x;
		double fy;
		reference_sample_axis(y, enlarged_height, destination_height, &y0, &y1, &fy);
		for (x = 0; x < destination_width; x++) {
			int x0, x1;
			double fx;
			reference_sample_axis(x, enlarged_width, destination_width, &x0, &x1, &fx);
			destination[(size_t) y * destination_width + x] = reference_blend(
				enlarged[(size_t) y0 * enlarged_width + x0],
				enlarged[(size_t) y0 * enlarged_width + x1],
				enlarged[(size_t) y1 * enlarged_width + x0],
				enlarged[(size_t) y1 * enlarged_width + x1], fx, fy);
		}
	}
	free(enlarged);
	return 1;
}

static int pixels_match_with_rounding_tolerance(uint32_t actual, uint32_t expected) {
	unsigned shift;
	for (shift = 0; shift <= 24; shift += 8) {
		int difference = (int) ((actual >> shift) & 0xff) - (int) ((expected >> shift) & 0xff);
		if (difference < -1 || difference > 1) return 0;
	}
	return 1;
}

static int test_integer_scale(void) {
	const uint32_t source[4] = {
		0xff0000ff, 0xff00ff00,
		0xffff0000, 0xffffffff
	};
	uint32_t output[16];
	int scale_x = 0;
	int scale_y = 0;
	int x, y;
	if (!zzt_sharp_bilinear_scale(source, 2, 2, output, 4, 4, &scale_x, &scale_y)
		|| scale_x != 2 || scale_y != 2) return 0;
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			if (output[y * 4 + x] != source[(y / 2) * 2 + (x / 2)]) return 0;
		}
	}
	return 1;
}

static int test_fractional_scale(void) {
	const uint32_t source[4] = {
		0xff000000, 0xffffffff,
		0xff000000, 0xffffffff
	};
	uint32_t output[9];
	int scale_x = 0;
	int scale_y = 0;
	int y;
	if (!zzt_sharp_bilinear_scale(source, 2, 2, output, 3, 3, &scale_x, &scale_y)
		|| scale_x != 1 || scale_y != 1) return 0;
	if ((output[1] & 0x00ffffff) < 0x007f7f7f || (output[1] & 0x00ffffff) > 0x00808080) return 0;
	if ((output[0] & 0x00ffffff) != 0 || (output[2] & 0x00ffffff) != 0x00ffffff) return 0;
	for (y = 0; y < 3; y++) {
		if (output[y * 3] != output[0] || output[y * 3 + 1] != output[1]
			|| output[y * 3 + 2] != output[2]) return 0;
	}
	return 1;
}

static int test_fractional_after_integer_stage(void) {
	const uint32_t source[4] = {
		0xff000000, 0xffffffff,
		0xff000000, 0xffffffff
	};
	uint32_t output[25];
	const uint32_t expected[5] = {
		0xff000000, 0xff000000, 0xff808080, 0xffffffff, 0xffffffff
	};
	int scale_x = 0;
	int scale_y = 0;
	int x, y;
	if (!zzt_sharp_bilinear_scale(source, 2, 2, output, 5, 5, &scale_x, &scale_y)
		|| scale_x != 2 || scale_y != 2) return 0;
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			if (output[y * 5 + x] != expected[x]) return 0;
		}
	}
	return 1;
}

static int test_materialized_nearest_then_bilinear_equivalence(void) {
	uint32_t *reference = (uint32_t *) malloc(sizeof(window_frame));
	int scale_x = 0;
	int scale_y = 0;
	int x, y;
	if (reference == NULL) return 0;
	for (y = 0; y < NATIVE_HEIGHT; y++) {
		for (x = 0; x < NATIVE_WIDTH; x++) {
			native_frame[y * NATIVE_WIDTH + x] = 0xff000000
				| ((uint32_t) (x * 13 + y * 7) & 0xff) << 16
				| ((uint32_t) (x ^ (y * 3)) & 0xff) << 8
				| ((uint32_t) (x * 5 + y * 11) & 0xff);
		}
	}
	if (!zzt_sharp_bilinear_scale(native_frame, NATIVE_WIDTH, NATIVE_HEIGHT,
		window_frame, WINDOW_WIDTH, WINDOW_HEIGHT, &scale_x, &scale_y)
		|| scale_x != 1 || scale_y != 2
		|| !materialized_nearest_then_bilinear(native_frame, NATIVE_WIDTH, NATIVE_HEIGHT,
			scale_x, scale_y, reference, WINDOW_WIDTH, WINDOW_HEIGHT)) {
		free(reference);
		return 0;
	}
	for (y = 0; y < WINDOW_HEIGHT; y++) {
		for (x = 0; x < WINDOW_WIDTH; x++) {
			size_t pixel = (size_t) y * WINDOW_WIDTH + x;
			if (!pixels_match_with_rounding_tolerance(window_frame[pixel], reference[pixel])) {
				free(reference);
				return 0;
			}
		}
	}
	free(reference);
	return 1;
}

static int test_ega_aspect_correction(void) {
	int scale_x = 0;
	int scale_y = 0;
	int i;
	for (i = 0; i < NATIVE_WIDTH * NATIVE_HEIGHT; i++) native_frame[i] = 0xff000000;
	/* One lit EGA scanline expands to a fractional number of display rows.
	   Bilinear correction should distribute it smoothly instead of making an
	   alternating 1/2-row nearest-neighbor staircase. */
	for (i = 0; i < NATIVE_WIDTH; i++) native_frame[174 * NATIVE_WIDTH + i] = 0xffffffff;
	if (!zzt_sharp_bilinear_scale(native_frame, NATIVE_WIDTH, NATIVE_HEIGHT,
		crt_frame, CRT_WIDTH, CRT_HEIGHT, &scale_x, &scale_y)
		|| scale_x != 1 || scale_y != 1) return 0;
	if ((crt_frame[238 * CRT_WIDTH + CRT_WIDTH / 2] & 0xff) != 104) return 0;
	if ((crt_frame[239 * CRT_WIDTH + CRT_WIDTH / 2] & 0xff) != 220) return 0;
	if ((crt_frame[240 * CRT_WIDTH + CRT_WIDTH / 2] & 0xff) != 35) return 0;
	if ((crt_frame[237 * CRT_WIDTH + CRT_WIDTH / 2] & 0xff) != 0
		|| (crt_frame[241 * CRT_WIDTH + CRT_WIDTH / 2] & 0xff) != 0) return 0;
	return 1;
}

static int test_aspect_correction_with_axis_specific_integer_stages(void) {
	int scale_x = 0;
	int scale_y = 0;
	int i;
	for (i = 0; i < NATIVE_WIDTH * NATIVE_HEIGHT; i++) native_frame[i] = 0xff000000;
	for (i = 0; i < NATIVE_WIDTH; i++) native_frame[174 * NATIVE_WIDTH + i] = 0xffffffff;
	if (!zzt_sharp_bilinear_scale(native_frame, NATIVE_WIDTH, NATIVE_HEIGHT,
		window_frame, WINDOW_WIDTH, WINDOW_HEIGHT, &scale_x, &scale_y)
		|| scale_x != 1 || scale_y != 2) return 0;
	/* At 960x720, vertical replication leaves only a 1.03x final filter,
	   while horizontal expansion remains the required 1.5x fractional scale. */
	if ((window_frame[357 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) < 16
		|| (window_frame[357 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) > 20) return 0;
	if ((window_frame[358 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) != 0xff) return 0;
	if ((window_frame[359 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) < 248
		|| (window_frame[359 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) > 255) return 0;
	if ((window_frame[360 * WINDOW_WIDTH + WINDOW_WIDTH / 2] & 0xff) > 8) return 0;
	return 1;
}

static int test_fullscreen_4_3_filter(void) {
	int scale_x = 0;
	int scale_y = 0;
	int i;
	int x = FULLSCREEN_WIDTH / 2;
	for (i = 0; i < NATIVE_WIDTH * NATIVE_HEIGHT; i++) native_frame[i] = 0xff000000;
	for (i = 0; i < NATIVE_WIDTH; i++) native_frame[174 * NATIVE_WIDTH + i] = 0xffffffff;
	if (!zzt_sharp_bilinear_scale(native_frame, NATIVE_WIDTH, NATIVE_HEIGHT,
		fullscreen_frame, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, &scale_x, &scale_y)
		|| scale_x != 2 || scale_y != 3) return 0;
	/* A 1440x1080 4:3 viewport keeps the same source aspect correction as the
	   windowed 960x720 case while using larger integer nearest-neighbor stages. */
	if ((fullscreen_frame[535 * FULLSCREEN_WIDTH + x] & 0xff) != 0) return 0;
	if ((fullscreen_frame[537 * FULLSCREEN_WIDTH + x] & 0xff) != 0xff) return 0;
	if ((fullscreen_frame[539 * FULLSCREEN_WIDTH + x] & 0xff) < 240) return 0;
	if ((fullscreen_frame[540 * FULLSCREEN_WIDTH + x] & 0xff) > 8) return 0;
	return 1;
}

static int test_4_3_viewport_fit(void) {
	int x, y, width, height;
	if (!zzt_fit_4_3_viewport(1920, 1080, &x, &y, &width, &height)
		|| x != 240 || y != 0 || width != 1440 || height != 1080) return 0;
	if (!zzt_fit_4_3_viewport(1080, 1920, &x, &y, &width, &height)
		|| x != 0 || y != 555 || width != 1080 || height != 810) return 0;
	if (!zzt_fit_4_3_viewport(960, 720, &x, &y, &width, &height)
		|| x != 0 || y != 0 || width != 960 || height != 720) return 0;
	if (zzt_fit_4_3_viewport(0, 720, &x, &y, &width, &height)
		|| zzt_fit_4_3_viewport(960, 720, NULL, &y, &width, &height)) return 0;
	return 1;
}

int main(void) {
	if (!test_integer_scale()) {
		fputs("integer nearest-neighbor scaling test failed\n", stderr);
		return 1;
	}
	if (!test_fractional_scale()) {
		fputs("fractional sharp-bilinear scaling test failed\n", stderr);
		return 1;
	}
	if (!test_fractional_after_integer_stage()) {
		fputs("fractional filtering after integer nearest-neighbor stage test failed\n", stderr);
		return 1;
	}
	if (!test_materialized_nearest_then_bilinear_equivalence()) {
		fputs("virtual and materialized 960x720 nearest-then-bilinear pipelines differ\n", stderr);
		return 1;
	}
	if (!test_ega_aspect_correction()) {
		fputs("640x350 to 4:3 scanline correction test failed\n", stderr);
		return 1;
	}
	if (!test_aspect_correction_with_axis_specific_integer_stages()) {
		fputs("960x720 axis-specific sharp scaling test failed\n", stderr);
		return 1;
	}
	if (!test_fullscreen_4_3_filter()) {
		fputs("1440x1080 fullscreen sharp scaling test failed\n", stderr);
		return 1;
	}
	if (!test_4_3_viewport_fit()) {
		fputs("windowed/fullscreen 4:3 viewport fit test failed\n", stderr);
		return 1;
	}
	puts("PASS: axis-specific integer nearest-neighbor stages, fractional scaling, and centered 4:3 viewport fitting");
	return 0;
}
