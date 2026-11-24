#include <stdint.h>
#include "zoo_random.h"

int32_t run_rng_checks(void) {
	static const int16_t tp55_expected[] = {343, 282, 986, 996, 781, 855, 343, 32};
	static const int16_t drum4_expected[] = {
		500, 2556, 1929, 3776, 3386, 4517, 1385,
		1103, 4895, 3396, 874, 1616, 5124, 606
	};
	static const int16_t drum5_expected[] = {1514, 821, 1715, 911, 1968, 1490, 1722, 1723};
	static const int16_t drum8_expected[] = {
		1207, 1224, 1163, 1127, 1159, 1236, 1269,
		1314, 1127, 1224, 1320, 1332, 1257, 1327
	};
	static const int16_t drum9_expected[] = {
		378, 331, 316, 230, 224, 384, 480,
		320, 358, 412, 376, 621, 554, 426
	};
	uint32_t seed = 123;
	uint32_t drum_seed = 0;
	uint32_t zero_seed = 0;
	uint16_t i;

	if (zoo_random_seed_from_dos_time(23, 59, 59, 99) != 0x3B63173BU) return 90;
	for (i = 0; i < sizeof(tp55_expected) / sizeof(tp55_expected[0]); i++) {
		if (zoo_random_next(&seed, 1000) != tp55_expected[i]) return 1 + i;
	}
	for (i = 0; i < sizeof(drum4_expected) / sizeof(drum4_expected[0]); i++) {
		if (zoo_random_next(&drum_seed, 5000) + 500 != drum4_expected[i]) return 20 + i;
	}
	for (i = 0; i < sizeof(drum5_expected) / sizeof(drum5_expected[0]); i++) {
		if (zoo_random_next(&drum_seed, 1600) + 800 != drum5_expected[i]) return 30 + i;
	}
	for (i = 0; i < sizeof(drum8_expected) / sizeof(drum8_expected[0]); i++) {
		if (((i + 1) * 20 + 1200) - zoo_random_next(&drum_seed, (i + 1) * 40)
				!= drum8_expected[i]) return 40 + i;
	}
	for (i = 0; i < sizeof(drum9_expected) / sizeof(drum9_expected[0]); i++) {
		if (zoo_random_next(&drum_seed, 440) + 220 != drum9_expected[i]) return 60 + i;
	}
	if (zoo_random_next(&zero_seed, 0) != 0 || zero_seed != 1) return 80;
	return 0;
}
