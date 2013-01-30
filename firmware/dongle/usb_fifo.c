#include "usb_fifo.h"
#include "assert.h"
#include "registers.h"
#include "stddef.h"
#include "usb_internal.h"

#define FIFO_MIN_SIZE (16 * 4)
#define TOTAL_FIFO_SPACE (1024 * 5 / 4)
#define NUM_FIFOS 4
static volatile uint32_t * const TX_FIFO_REGISTERS[NUM_FIFOS] = { &OTG_FS_DIEPTXF0, &OTG_FS_DIEPTXF1, &OTG_FS_DIEPTXF2, &OTG_FS_DIEPTXF3 };

static size_t total_fifo_space_used(void) {
	size_t acc = 0;
	for (unsigned int i = 0; i < NUM_FIFOS; ++i) {
		acc += usb_fifo_get_size(i);
	}
	return acc;
}

void usb_fifo_init(size_t fifo_zero_size) {
	// Set receive FIFO to proper size.
	OTG_FS_GRXFSIZ = RXFD(usb_device_info->rx_fifo_words);

	// Set transmit FIFO 0 to proper size, immediately after receive FIFO.
	OTG_FS_DIEPTXF0 = TX0FD(fifo_zero_size) | TX0FSA(usb_device_info->rx_fifo_words);

	// Set remaining FIFOs to 16 words each, packed into consecutive memory areas.
	size_t offset = usb_device_info->rx_fifo_words + fifo_zero_size;
	for (size_t i = 1; i < NUM_FIFOS; ++i) {
		*TX_FIFO_REGISTERS[i] = INEPTXFD(16) | INEPTXSA(offset);
		offset += 16;
	}
}

void usb_fifo_reset(void) {
	for (unsigned int i = 1; i < NUM_FIFOS; ++i) {
		usb_fifo_set_size(i, FIFO_MIN_SIZE);
		usb_fifo_flush(i);
	}
}

size_t usb_fifo_get_offset(unsigned int fifo) {
	return INEPTXSA_X(*TX_FIFO_REGISTERS[fifo]);
}

size_t usb_fifo_get_size(unsigned int fifo) {
	return INEPTXFD_X(*TX_FIFO_REGISTERS[fifo]) * 4;
}

void usb_fifo_set_size(unsigned int fifo, size_t size) {
	// Sanity check the size and convert to words.
	assert(!(size % 4));
	assert(size >= FIFO_MIN_SIZE);
	size /= 4;

	// Keep this FIFO’s offset the same, but change its size.
	size_t offset = usb_fifo_get_offset(fifo);
	*TX_FIFO_REGISTERS[fifo] = INEPTXFD(size) | INEPTXSA(offset);

	// Advance past this FIFO.
	offset += size;

	// The remaining FIFOs keep the same size but change offsets.
	for (unsigned int i = fifo + 1; i < NUM_FIFOS; ++i) {
		size = usb_fifo_get_size(i) / 4;
		*TX_FIFO_REGISTERS[fifo] = INEPTXFD(size) | INEPTXSA(offset);
		offset += size;
	}

	// Sanity check that we have not overrun the available FIFO memory.
	assert(total_fifo_space_used() <= TOTAL_FIFO_SPACE);
}

void usb_fifo_flush(unsigned int fifo) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!(OTG_FS_GRSTCTL & AHBIDL));

	// Flush the FIFO.
	OTG_FS_GRSTCTL = GRSTCTL_TXFNUM(fifo) | TXFFLSH;

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL & TXFFLSH);
}

void usb_fifo_rx_flush(void) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!(OTG_FS_GRSTCTL & AHBIDL));

	// Flush the FIFO.
	OTG_FS_GRSTCTL = RXFFLSH;

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL & RXFFLSH);
}

