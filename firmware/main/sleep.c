#include "sleep.h"
#include "tsc.h"

void sleep_short(void) {
	uint32_t start = rdtsc();
	while (rdtsc() - start < F_CPU / 200U);
}

void sleep_1s(void) {
	uint32_t start = rdtsc();
	while (rdtsc() - start < F_CPU);
}

