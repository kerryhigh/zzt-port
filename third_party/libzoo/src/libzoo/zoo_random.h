#ifndef ZOO_RANDOM_H
#define ZOO_RANDOM_H

#include <stdint.h>

static inline uint32_t zoo_random_seed_from_dos_time(
	uint8_t hour, uint8_t minute, uint8_t second, uint8_t hundredth
) {
	uint16_t cx = ((uint16_t) hour << 8) | minute;
	uint16_t dx = ((uint16_t) second << 8) | hundredth;
	return ((uint32_t) dx << 16) | cx;
}

static inline int16_t zoo_random_next(uint32_t *seed, int16_t max) {
	uint16_t random_value;
	uint16_t range;

	*seed = (*seed * 134775813U) + 1U;
	if (max <= 0) return 0;
	range = (uint16_t) max;
	// Turbo Pascal 5.5 reduces the updated seed's high Word modulo Range.
	// Turbo Pascal 7 changed this to scaling the full 32-bit seed.
	random_value = (uint16_t) (*seed >> 16);
	return (int16_t) (random_value % range);
}

#endif
