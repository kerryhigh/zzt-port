#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../../third_party/libzoo/src/libzoo/zoo_sound.c"

static zoo_sound_driver sound_driver;
static zoo_sound_state sound_state;
static uint16_t recorded_pitch_freqs[72];
static uint16_t recorded_pitch_count;

void *memset(void *dest, int value, size_t count) {
	uint8_t *bytes = (uint8_t *) dest;
	size_t i;
	for (i = 0; i < count; i++) bytes[i] = (uint8_t) value;
	return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
	uint8_t *d = (uint8_t *) dest;
	const uint8_t *s = (const uint8_t *) src;
	size_t i;
	for (i = 0; i < count; i++) d[i] = s[i];
	return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
	uint8_t *d = (uint8_t *) dest;
	const uint8_t *s = (const uint8_t *) src;
	if ((uintptr_t) d < (uintptr_t) s) {
		size_t i;
		for (i = 0; i < count; i++) d[i] = s[i];
	} else if ((uintptr_t) d > (uintptr_t) s) {
		while (count > 0) {
			count--;
			d[count] = s[count];
		}
	}
	return dest;
}

static void test_play_freqs(zoo_sound_driver *driver, const uint16_t *freqs,
		uint16_t len, bool clear) {
	(void) driver;
	if (!clear && freqs != NULL) {
		uint16_t i;
		for (i = 0; i < len && recorded_pitch_count < 72; i++) {
			recorded_pitch_freqs[recorded_pitch_count++] = freqs[i];
		}
	}
}

static void reset_sound(const uint8_t *pattern, int16_t len) {
	recorded_pitch_count = 0;
	zoo_sound_state_init(&sound_state);
	sound_driver.func_play_freqs = test_play_freqs;
	sound_state.d_sound = &sound_driver;
	zoo_sound_queue(&sound_state, 1, pattern, len);
}

int run_sound_tick_checks(void) {
	static const uint8_t one_byte[] = {0};
	static const uint8_t odd_pattern[] = {0, 1, 64};
	static const uint8_t even_pattern[] = {0, 1};
	static const uint8_t zero_duration_pattern[] = {0, 0, 64, 1};
	int i;

	reset_sound(one_byte, sizeof(one_byte));
	zoo_sound_tick(&sound_state);
	if (sound_state.is_playing || sound_state.buffer_pos != 0
			|| sound_state.duration_counter != 0) return 1;

	reset_sound(odd_pattern, sizeof(odd_pattern));
	zoo_sound_tick(&sound_state);
	if (!sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 1) return 2;
	zoo_sound_tick(&sound_state);
	if (sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 0) return 3;

	reset_sound(even_pattern, sizeof(even_pattern));
	zoo_sound_tick(&sound_state);
	if (!sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 1) return 4;
	zoo_sound_tick(&sound_state);
	if (sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 0) return 5;

	reset_sound(zero_duration_pattern, sizeof(zero_duration_pattern));
	zoo_sound_tick(&sound_state);
	if (!sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 0) return 6;
	for (i = 0; i < 255; i++) zoo_sound_tick(&sound_state);
	if (!sound_state.is_playing || sound_state.buffer_pos != 2
			|| sound_state.duration_counter != 1) return 7;
	zoo_sound_tick(&sound_state);
	if (!sound_state.is_playing || sound_state.buffer_pos != 4
			|| sound_state.duration_counter != 1) return 8;

	return 0;
}

int run_sound_pitch_table_checks(void) {
	uint8_t pattern[72 * 2];
	uint16_t note_index = 0;
	uint8_t octave, note;

	for (octave = 1; octave <= 6; octave++) {
		for (note = 0; note < 12; note++) {
			pattern[note_index * 2] = (uint8_t) (octave * 16 + note);
			pattern[note_index * 2 + 1] = 1;
			note_index++;
		}
	}
	if (note_index != 72) return 1;

	reset_sound(pattern, (int16_t) sizeof(pattern));
	for (note_index = 0; note_index < 72; note_index++) {
		zoo_sound_tick(&sound_state);
	}
	if (recorded_pitch_count != 72) return 2;
	if (recorded_pitch_freqs[0] != 64 || recorded_pitch_freqs[11] != 120
			|| recorded_pitch_freqs[12] != 128 || recorded_pitch_freqs[71] != 3866) return 3;
	return 0;
}

uint32_t sound_pitch_output_ptr(void) {
	return (uint32_t) (uintptr_t) recorded_pitch_freqs;
}
