#include "flash.h"
#include "io.h"
#include <stdint.h>

static void assert_cs(void) {
	outb(FLASH_CTL, 0x00);
}

static void deassert_cs(void) {
	outb(FLASH_CTL, 0x02);
}

static void tx(uint8_t b) {
	outb(FLASH_DATA, b);
	while (inb(FLASH_CTL) & 0x01);
}

static uint8_t txrx(uint8_t b) {
	tx(b);
	return inb(FLASH_DATA);
}

static void write_enable(void) {
	assert_cs();
	tx(0x06);
	deassert_cs();
}

static void wait_not_busy(void) {
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	assert_cs();
	tx(0x05);
	while (txrx(0x00) & 0x01);
	deassert_cs();
}

void flash_erase_sector(uint32_t address) {
	write_enable();

	assert_cs();
	tx(0x20);
	tx((uint8_t) (address >> 16));
	tx((uint8_t) (address >> 8));
	tx((uint8_t) address);
	deassert_cs();

	wait_not_busy();
}

void flash_page_program(uint16_t page, const void *data, uint8_t length) {
	write_enable();

	assert_cs();
	tx(0x02);
	tx(page >> 8);
	tx(page);
	tx(0x00);
	const uint8_t *p = data;
	do {
		tx(*p++);
	} while (--length);
	deassert_cs();

	wait_not_busy();
}

void flash_read(uint32_t address, void *data, uint8_t length) {
	assert_cs();
	tx(0x03);
	tx(address >> 16);
	tx(address >> 8);
	tx(address);
	uint8_t *ptr = data;
	while (length--) {
		*ptr++ += txrx(0x00);
	}
	deassert_cs();
}

