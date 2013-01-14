#include "usb_fifo.h"
#include "registers.h"
#include "stddef.h"
#include "usb_internal.h"

#define FIFO_MIN_SIZE 16
#define NUM_FIFOS 4
static volatile uint32_t * const TX_FIFO_REGISTERS[NUM_FIFOS] = { &OTG_FS_DIEPTXF0, &OTG_FS_DIEPTXF1, &OTG_FS_DIEPTXF2, &OTG_FS_DIEPTXF3 };

void usb_fifo_init(size_t fifo_zero_size) {
	// Set receive FIFO to proper size.
	OTG_FS_GRXFSIZ = (OTG_FS_GRXFSIZ & 0xFFFF0000) | usb_device_info->rx_fifo_words;

	// Set transmit FIFO 0 to proper size, immediately after receive FIFO.
	OTG_FS_DIEPTXF0 = (fifo_zero_size << 16) | usb_device_info->rx_fifo_words;

	// Set remaining FIFOs to 16 words each, packed into consecutive memory areas.
	size_t offset = usb_device_info->rx_fifo_words + fifo_zero_size;
	for (size_t i = 1; i < NUM_FIFOS; ++i) {
		*TX_FIFO_REGISTERS[i] = (16 << 16) | offset;
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
	return *TX_FIFO_REGISTERS[fifo] & 0xFFFF;
}

size_t usb_fifo_get_size(unsigned int fifo) {
	return *TX_FIFO_REGISTERS[fifo] >> 16;
}

void usb_fifo_set_size(unsigned int fifo, size_t size) {
	// Keep this FIFO’s offset the same, but change its size.
	size_t offset = usb_fifo_get_offset(fifo);
	*TX_FIFO_REGISTERS[fifo] = (size << 16) | offset;

	// Advance past this FIFO.
	offset += size;

	// The remaining FIFOs keep the same size but change offsets.
	for (unsigned int i = fifo + 1; i < NUM_FIFOS; ++i) {
		size = usb_fifo_get_size(i);
		*TX_FIFO_REGISTERS[fifo] = (size << 16) | offset;
		offset += size;
	}
}

void usb_fifo_flush(unsigned int fifo) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));

	// Flush the FIFO.
	OTG_FS_GRSTCTL =
		(OTG_FS_GRSTCTL & 0x7FFFF808) // Reserved bits
		| (fifo << 6); // TXFNUM = fifo; select transmit FIFO to flush
	OTG_FS_GRSTCTL |= 1 << 5; // TXFLSH = 1; initiate FIFO flush

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFLSH */);
}

void usb_fifo_rx_flush(void) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));

	// Flush the FIFO.
	OTG_FS_GRSTCTL |= 1 << 4; // RXFLSH = 1; initiate FIFO flush

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL & (1 << 4) /* RXFLSH */);
}

