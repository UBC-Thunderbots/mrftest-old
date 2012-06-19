#include "io.h"
#include <stdio.h>

static int put_function(char ch, FILE *fp __attribute__((unused))) {
	outb(DEBUG_CTL, 0x01);
	outb(DEBUG_DATA, ch);
	while (inb(DEBUG_CTL) & 0x02);
	outb(DEBUG_CTL, 0x00);
	return 0;
}

static FILE fp;

void debug_init(void) {
	fdev_setup_stream(&fp, &put_function, 0, _FDEV_SETUP_WRITE);
	stdout = &fp;
	stderr = &fp;
}

