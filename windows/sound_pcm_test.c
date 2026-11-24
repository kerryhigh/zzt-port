#include "zoo_sound_pcm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static zoo_sound_pcm_driver pcm;
static uint8_t samples[64];

static void prepare_pcm(void) {
	memset(&pcm, 0, sizeof(pcm));
	pcm.frequency = 22050;
	pcm.channels = 1;
	pcm.volume = 200;
	pcm.format_signed = false;
	pcm.latency = 1;
	zoo_sound_pcm_init(&pcm);
}

int main(void) {
	static const uint16_t drum_frequencies[] = {440, 880};
	static const uint16_t short_pc_speaker_pulse = 110;
	prepare_pcm();
	pcm.parent.func_play_freqs(&pcm.parent, drum_frequencies,
		(uint16_t) (sizeof(drum_frequencies) / sizeof(drum_frequencies[0])), true);
	pcm.ticks = 3;
	pcm.buf_ticks = 2;
	zoo_sound_pcm_generate(&pcm, samples, sizeof(samples));
	if (samples[0] != 28 || samples[22] != 28 || samples[23] != 228) {
		fputs("drum frequencies did not start immediately or change at the 1 ms boundary\n", stderr);
		return 1;
	}

	prepare_pcm();
	pcm.parent.func_play_freqs(&pcm.parent, &short_pc_speaker_pulse, 1, false);
	pcm.parent.func_play_freqs(&pcm.parent, NULL, 0, false);
	pcm.ticks = 3;
	pcm.buf_ticks = 2;
	zoo_sound_pcm_generate(&pcm, samples, sizeof(samples));
	if (samples[0] != 28 || samples[1] != 128) {
		fputs("same-tick Sound(110); NoSound did not retain a one-sample pulse\n", stderr);
		return 2;
	}

	puts("PASS: immediate drum sequence and one-sample 110 Hz pulse match Pascal sound timing");
	return 0;
}
