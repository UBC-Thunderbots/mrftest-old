#include <stdlib.h>

// This overrides the abort function from newlib.
void abort(void) {
	asm volatile("bkpt");
	for (;;);
}

