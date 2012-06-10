#include "io.h"
#include "run.h"

static unsigned char stack[1024] __attribute__((section(".stack"), used));

static void entry(void) __attribute__((section(".entry"), used, naked));
static void entry(void) {
	/* Initialize the stack pointer and jump to avr_main */
	asm volatile(
		"ldi r16, 0xFF\n\t"
		"out 0x3D, r16\n\t"
		"ldi r16, 0x0C\n\t"
		"out 0x3E, r16\n\t"
		"rjmp avr_main\n\t");
}

static void avr_main(void) __attribute__((noreturn, used));
static void avr_main(void) {
	startup();
	for(;;) {
		uint8_t tick_count = inb(TICKS);
		tick();
		do{fast_poll();} while(inb(TICKS) == tick_count);
	}
}

