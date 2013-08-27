#include "sleep.h"
#include "io.h"

void sleep_short(void) {
	unsigned int start = IO_SYSCTL.tsc;
	while (IO_SYSCTL.tsc - start < F_CPU / 200U);
}

void sleep_1s(void) {
	unsigned int start = IO_SYSCTL.tsc;
	while (IO_SYSCTL.tsc - start < F_CPU);
}

