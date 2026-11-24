#include <stddef.h>
#include <stdint.h>
#include "../../third_party/libzoo/src/libzoo/zoo_sound.c"

static char sound_input[256];
static uint8_t sound_output[256];

uint32_t sound_input_ptr(void) {
	return (uint32_t)(uintptr_t)sound_input;
}

uint32_t sound_output_ptr(void) {
	return (uint32_t)(uintptr_t)sound_output;
}

int32_t run_sound_parse_from_input(uint32_t input_len) {
	uint32_t i;
	if (input_len > 255) return -1;
	sound_input[input_len] = '\0';
	for (i = 0; i < sizeof(sound_output); i++) sound_output[i] = 0;
	return zoo_sound_parse(sound_input, sound_output, sizeof(sound_output));
}

size_t strlen(const char *text) {
	const char *end = text;
	while (*end) end++;
	return (size_t)(end - text);
}

static int matches(const char *input, const uint8_t *expected, int16_t expected_len) {
	uint8_t actual[8] = {0};
	int16_t actual_len = zoo_sound_parse(input, actual, sizeof(actual));
	int16_t i;
	if (actual_len != expected_len) return 0;
	for (i = 0; i < expected_len; i++) {
		if (actual[i] != expected[i]) return 0;
	}
	return 1;
}

int run_sound_parse_checks(void) {
	static const uint8_t c_flat[] = {47, 1};
	static const uint8_t b_sharp[] = {60, 1};
	static const uint8_t long_dotted[] = {48, 198};
	static const uint8_t overflow_dotted[] = {48, 220};

	if (!matches("C!", c_flat, sizeof(c_flat))) return 1;
	if (!matches("B#", b_sharp, sizeof(b_sharp))) return 2;
	if (!matches("Q..........C", long_dotted, sizeof(long_dotted))) return 3;
	if (!matches("W....................C", overflow_dotted, sizeof(overflow_dotted))) return 4;
	return 0;
}
