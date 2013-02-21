#include <usb_fifo.h>
#include <assert.h>
#include <registers.h>

#define FIFO_MIN_SIZE (16 * 4)
#define TOTAL_FIFO_SPACE (1024 * 5 / 4)
#define NUM_FIFOS 4
static volatile uint32_t * const TX_FIFO_REGISTERS[NUM_FIFOS] = { &OTG_FS_DIEPTXF0, &OTG_FS_DIEPTXF1, &OTG_FS_DIEPTXF2, &OTG_FS_DIEPTXF3 };

static size_t total_fifo_space_used(void) {
	size_t acc = RXFD_X(OTG_FS_GRXFSIZ);
	for (unsigned int i = 0; i < NUM_FIFOS; ++i) {
		acc += INEPTXFD_X(*TX_FIFO_REGISTERS[i]);
	}
	return acc * 4;
}

static void usb_fifo_rx_flush(void) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!(OTG_FS_GRSTCTL & AHBIDL));

	// Flush the FIFO.
	OTG_FS_GRSTCTL = RXFFLSH;

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL & RXFFLSH);
}

void usb_fifo_init(size_t rx_size, size_t tx0_size) {
	// Sanity check.
	assert(rx_size >= FIFO_MIN_SIZE);
	assert(!(rx_size % 4));
	assert(tx0_size >= FIFO_MIN_SIZE);
	assert(!(tx0_size % 4));
	assert(rx_size + tx0_size <= TOTAL_FIFO_SPACE);

	// Set receive FIFO to proper size.
	OTG_FS_GRXFSIZ = RXFD(rx_size / 4);

	// Set transmit FIFO 0 to proper size, immediately after receive FIFO.
	OTG_FS_DIEPTXF0 = TX0FD(tx0_size / 4) | TX0FSA(rx_size / 4);

	// Zero out the rest of the FIFOs.
	for (unsigned int i = 1; i < NUM_FIFOS; ++i) {
		*TX_FIFO_REGISTERS[i] = 0;
	}

	// Flush the FIFOs we actually created.
	usb_fifo_flush(0);
	usb_fifo_rx_flush();
}

void usb_fifo_enable(unsigned int fifo, size_t size) {
	// Sanity check.
	assert(fifo);
	assert(fifo < NUM_FIFOS);
	assert(size >= FIFO_MIN_SIZE);
	assert(!(size % 4));
	assert(!*TX_FIFO_REGISTERS[fifo]);
	assert(*TX_FIFO_REGISTERS[fifo - 1]);

	// Check for available space.
	size_t used = total_fifo_space_used();
	assert(used + size <= TOTAL_FIFO_SPACE);

	// Enable and flush the FIFO.
	*TX_FIFO_REGISTERS[fifo] = INEPTXFD(size / 4) | INEPTXSA(used / 4);
	usb_fifo_flush(fifo);
}

void usb_fifo_disable(unsigned int fifo) {
	// Sanity checks.
	assert(fifo);
	assert(fifo < NUM_FIFOS);
	assert(fifo == NUM_FIFOS - 1 || !*TX_FIFO_REGISTERS[fifo + 1]);

	// Disable the FIFO.
	*TX_FIFO_REGISTERS[fifo] = 0;
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

size_t usb_fifo_get_size(unsigned int fifo) {
	// Sanity check.
	assert(fifo < NUM_FIFOS);

	// Do it.
	return INEPTXFD_X(*TX_FIFO_REGISTERS[fifo]);
}

