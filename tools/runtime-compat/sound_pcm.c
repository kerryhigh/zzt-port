#include <stddef.h>
#include <stdint.h>
#include "zoo_sound_pcm.h"

static zoo_sound_pcm_driver silence_pcm;
static zoo_sound_pcm_driver tone_pcm;
static uint8_t samples[64];
static const uint16_t zero_frequencies[14] = {0};
static const uint16_t tone_frequency = 440;

static void prepare(zoo_sound_pcm_driver *pcm) {
	pcm->frequency = 22050;
	pcm->channels = 1;
	pcm->volume = 200;
	pcm->format_signed = false;
	pcm->latency = 1;
	zoo_sound_pcm_init(pcm);
}

uint32_t run_sound_pcm_checks(void) {
	uint32_t i;
	prepare(&silence_pcm);
	silence_pcm.parent.func_play_freqs(&silence_pcm.parent, zero_frequencies,
		(uint16_t) (sizeof(zero_frequencies) / sizeof(zero_frequencies[0])), true);
	// Place the audio cursor at the first complete tick after the queued
	// sound event, with exactly one tick of latency, as the Android mixer does.
	silence_pcm.ticks = 3;
	silence_pcm.buf_ticks = 2;
	zoo_sound_pcm_generate(&silence_pcm, samples, sizeof(samples));
	for (i = 0; i < sizeof(samples); i++) {
		if (samples[i] != 0x80) return 1 + i;
	}

	prepare(&tone_pcm);
	tone_pcm.parent.func_play_freqs(&tone_pcm.parent, &tone_frequency, 1, false);
	tone_pcm.ticks = 3;
	tone_pcm.buf_ticks = 2;
	zoo_sound_pcm_generate(&tone_pcm, samples, sizeof(samples));
	if (samples[0] != 28) return 100;
	if (samples[25] != 28) return 101;
	if (samples[26] != 228) return 102;
	if (samples[51] != 28) return 103;
	return 0;
}

void *memset(void *dest, int value, size_t size) {
	uint8_t *bytes = (uint8_t *) dest;
	size_t i;
	for (i = 0; i < size; i++) bytes[i] = (uint8_t) value;
	return dest;
}
