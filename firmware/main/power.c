#include "power.h"
#include "io.h"

void power_reboot(void) {
	static const uint16_t COMMANDS[] = {
		0xFFFF, // Dummy word
		0xAA99, // Sync word
		0x5566, // Sync word
		0x3261, // Type 1 write 1 word to GENERAL1
		0x0000, // Multiboot start address LSW 0x0000
		0x3281, // Type 1 write 1 word to GENERAL2
		0x0300, // Multiboot opcode 0x03 and address MSB 0x00
		0x32A1, // Type 1 write 1 word to GENERAL3
		0x0000, // Fallback start address LSW 0x0000
		0x32C1, // Type 1 write 1 word to GENERAL4
		0x0300, // Fallback opcode 0x03 and address MSB 0x00
		0x30A1, // Type 1 write 1 word to CMD
		0x000E, // IPROG command
		0x2000, // Type 1 NOP
	};

	for (uint8_t i = 0; i < sizeof(COMMANDS) / sizeof(*COMMANDS); ++i) {
		outb(ICAP_MSB, COMMANDS[i] >> 8);
		outb(ICAP_LSB, COMMANDS[i]);
		while (inb(ICAP_CTL) & 0x01);
	}

	for (;;);
}

