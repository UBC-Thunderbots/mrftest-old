#include "io.h"

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

static void sleep_1s(void) {
	uint8_t x = 200;
	while (--x) {
		uint8_t old = inb(TICKS);
		while (inb(TICKS) != old);
	}
}

static void avr_main(void) __attribute__((noreturn, used));
static void avr_main(void) {
	outb(FLASH_CTL, 0x00);
	outb(FLASH_DATA, 0x9F);
	while (inb(FLASH_CTL) & 0x01);
	outb(FLASH_DATA, 0x00);
	while (inb(FLASH_CTL) & 0x01);
	uint8_t mfgr_id = inb(FLASH_DATA);
	outb(FLASH_CTL, 0x02);
	if (mfgr_id == 0xEF) {
		outb(LED_CTL, 0x21);
	} else {
		outb(LED_CTL, 0x22);
	}
	for (;;);
}

