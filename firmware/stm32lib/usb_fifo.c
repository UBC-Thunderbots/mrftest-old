#include <usb_fifo.h>
#include <assert.h>
#include <registers/otg_fs.h>

#define FIFO_MIN_SIZE (16 * 4)
#define TOTAL_FIFO_SPACE (1024 * 5 / 4)
#define NUM_FIFOS 4

static size_t total_fifo_space_used(void) {
	size_t acc = OTG_FS_GRXFSIZ.RXFD;
	acc += OTG_FS_DIEPTXF0.TX0FD;
	for (unsigned int i = 1; i < NUM_FIFOS; ++i) {
		acc += OTG_FS_DIEPTXF[i].INEPTXFD;
	}
	return acc * 4;
}

static void usb_fifo_rx_flush(void) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!OTG_FS_GRSTCTL.AHBIDL);

	// Flush the FIFO.
	{
		OTG_FS_GRSTCTL_t tmp = { .RXFFLSH = 1 };
		OTG_FS_GRSTCTL = tmp;
	}

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL.RXFFLSH);
}

void usb_fifo_init(size_t rx_size, size_t tx0_size) {
	// Sanity check.
	assert(rx_size >= FIFO_MIN_SIZE);
	assert(!(rx_size % 4));
	assert(tx0_size >= FIFO_MIN_SIZE);
	assert(!(tx0_size % 4));
	assert(rx_size + tx0_size <= TOTAL_FIFO_SPACE);

	// Set receive FIFO to proper size.
	{
		OTG_FS_GRXFSIZ_t tmp = { .RXFD = rx_size / 4 };
		OTG_FS_GRXFSIZ = tmp;
	}

	// Set transmit FIFO 0 to proper size, immediately after receive FIFO.
	{
		OTG_FS_DIEPTXF0_t tmp = {
			.TX0FD = tx0_size / 4,
			.TX0FSA = rx_size / 4,
		};
		OTG_FS_DIEPTXF0 = tmp;
	}

	// Zero out the rest of the FIFOs.
	for (unsigned int i = 1; i < NUM_FIFOS; ++i) {
		OTG_FS_DIEPTXFx_t tmp = { 0 };
		OTG_FS_DIEPTXF[i] = tmp;
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
	assert(!OTG_FS_DIEPTXF[fifo].INEPTXFD);
	assert(!OTG_FS_DIEPTXF[fifo].INEPTXSA);
	if (fifo > 1) {
		assert(OTG_FS_DIEPTXF[fifo - 1].INEPTXFD);
	}

	// Check for available space.
	size_t used = total_fifo_space_used();
	assert(used + size <= TOTAL_FIFO_SPACE);

	// Enable and flush the FIFO.
	{
		OTG_FS_DIEPTXFx_t tmp = {
			.INEPTXFD = size / 4,
			.INEPTXSA = used / 4,
		};
		OTG_FS_DIEPTXF[fifo] = tmp;
	}
	usb_fifo_flush(fifo);
}

void usb_fifo_disable(unsigned int fifo) {
	// Sanity checks.
	assert(fifo);
	assert(fifo < NUM_FIFOS);
	assert(fifo == NUM_FIFOS - 1 || !OTG_FS_DIEPTXF[fifo + 1].INEPTXFD);

	// Disable the FIFO.
	{
		OTG_FS_DIEPTXFx_t tmp = { 0 };
		OTG_FS_DIEPTXF[fifo] = tmp;
	}
}

void usb_fifo_flush(unsigned int fifo) {
	// We assume that a relevant NAK is effective, because this is a precondition.
	// The other requirement is that the AHB be idle; wait for that now.
	while (!OTG_FS_GRSTCTL.AHBIDL);

	// Flush the FIFO.
	{
		OTG_FS_GRSTCTL_t tmp = {
			.TXFNUM = fifo,
			.TXFFLSH = 1,
		};
		OTG_FS_GRSTCTL = tmp;
	}

	// Wait until the flush is finished.
	while (OTG_FS_GRSTCTL.TXFFLSH);
}

size_t usb_fifo_get_size(unsigned int fifo) {
	// Sanity check.
	assert(fifo);
	assert(fifo < NUM_FIFOS);

	// Do it.
	return OTG_FS_DIEPTXF[fifo].INEPTXFD;
}

