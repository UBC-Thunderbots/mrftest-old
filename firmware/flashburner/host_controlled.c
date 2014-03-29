#include "host_controlled.h"
#include "autonomous.h"
#include "deferred.h"
#include "constants.h"
#include "io.h"
#include "spi.h"
#include <gpio.h>
#include <minmax.h>
#include <rcc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unused.h>
#include "usb_bi_in.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"
#include "usb_fifo.h"

#define PAGE_BYTES 256
#define NUM_PAGES (2 * 1024 * 1024 / PAGE_BYTES)

const uint8_t TARGET_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	USB_DTYPE_CONFIGURATION, // bDescriptorType
	34, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	2, // bConfigurationValue
	STRING_INDEX_CONFIG2, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFF, // bInterfaceClass
	FLASH_BURNER_POLLED_TARGET_SUBCLASS, // bInterfaceSubClass
	FLASH_BURNER_BURN_PROTOCOL, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	1, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	FLASH_BURNER_INTERRUPT_TARGET_SUBCLASS, // bInterfaceSubClass
	FLASH_BURNER_BURN_PROTOCOL, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	USB_DTYPE_ENDPOINT, // bDescriptorType
	0x81, // bEndpointAddress
	0x03, // bmAttributes
	1, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval
};

const uint8_t ONBOARD_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	USB_DTYPE_CONFIGURATION, // bDescriptorType
	34, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	3, // bConfigurationValue
	STRING_INDEX_CONFIG3, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFF, // bInterfaceClass
	FLASH_BURNER_POLLED_ONBOARD_SUBCLASS, // bInterfaceSubClass
	FLASH_BURNER_BURN_PROTOCOL, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	1, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	FLASH_BURNER_INTERRUPT_ONBOARD_SUBCLASS, // bInterfaceSubClass
	FLASH_BURNER_BURN_PROTOCOL, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	USB_DTYPE_ENDPOINT, // bDescriptorType
	0x81, // bEndpointAddress
	0x03, // bmAttributes
	1, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval
};

static volatile bool interrupt_mode, interrupt_monitoring;
static const struct spi_ops *spi;
static uint16_t write_page;
static uint8_t write_buffer[PAGE_BYTES];
static size_t read_bytes_left;
static deferred_fn_t monitor_deferred_fn;

static uint8_t read_status_register(void) {
	spi->assert_cs();
	spi->transceive_byte(0x05);
	uint8_t status = spi->transceive_byte(0);
	spi->deassert_cs();
	return status;
}

static void monitor_poll(void *UNUSED(cookie)) {
	if (!interrupt_monitoring) {
		// Monitoring has been cancelled; do nothing.
		// Do not reregister the deferred function call.
		return;
	}

	// A write or erase has been requested and the module is set up to autopoll and report completion on interrupt IN endpoint 1.
	// Poll the status now.
	uint8_t status = read_status_register();
	if (status & 1) {
		// The write is still in progress.
		// Reregister the deferred function call.
		deferred_fn_register(&monitor_deferred_fn, &monitor_poll, 0);
	} else {
		// The write has completed.
		// Stop monitoring now.
		interrupt_monitoring = false;

		// Start a transfer on the endpoint to notify the host.
		if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_IDLE) {
			usb_bi_in_start_transfer(1, 0, 1, 0, 0);
		}
	}
}

static void start_monitoring(void) {
	if (!interrupt_monitoring) {
		interrupt_monitoring = true;
		deferred_fn_register(&monitor_deferred_fn, &monitor_poll, 0);
	}
}

static void stop_monitoring(void) {
	if (interrupt_monitoring) {
		interrupt_monitoring = false;
		deferred_fn_register(&monitor_deferred_fn, 0, 0);
	}
}

static void handle_ep1i_set_or_clear_halt(unsigned int UNUSED(ep)) {
	// This is one way to stop monitoring an operation.
	stop_monitoring();
}

static void stop_read(void) {
	// If a read was in progress, stop it.
	if (read_bytes_left) {
		spi->deassert_cs();
		read_bytes_left = 0;
	}
}

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	// Any request at all indicates that the previous request has ended.
	// If that request was a read, stop the read.
	stop_read();

	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_ERASE) {
		// This request must have value set to a legal page number.
		if (pkt->value >= NUM_PAGES) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request cannot be issued while monitoring in interrupt mode.
		if (interrupt_monitoring) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request uses the SPI bus.
		spi->drive_bus();

		// This request cannot be issued if the chip is already busy.
		if (read_status_register() & 1) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Issue the WRITE ENABLE instruction.
		spi->assert_cs();
		spi->transceive_byte(0x06);
		spi->deassert_cs();

		// Issue the BLOCK ERASE 64KB instruction.
		spi->assert_cs();
		spi->transceive_byte(0xD8);
		spi->transceive_byte(pkt->value >> 8);
		spi->transceive_byte(pkt->value);
		spi->transceive_byte(0);
		spi->deassert_cs();

		// The chip must become busy as a result of the erase starting.
		if (!(read_status_register() & 1)) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// If in interrupt mode, start monitoring the erase.
		if (interrupt_mode) {
			start_monitoring();
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_SET_INTERFACE) {
		// Even if the alternate setting is not actually changing, this is a way to stop monitoring an operation.
		// Of course, if it is changing, we definitely need to do this.
		stop_monitoring();

		if (pkt->value == 0) {
			// Switching to polled mode.
			interrupt_mode = false;
			usb_bi_in_deinit(1);
		} else if (pkt->value == 1) {
			// Switching to interrupt mode.
			interrupt_mode = true;
			if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_HALTED) {
				usb_bi_in_clear_halt(1);
			}
			if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_ACTIVE) {
				usb_bi_in_abort_transfer(1);
			}
			if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_UNINITIALIZED) {
				usb_bi_in_init(1, 8, USB_BI_IN_EP_TYPE_INTERRUPT);
				usb_bi_in_set_std_halt(1, &handle_ep1i_set_or_clear_halt, 0, &handle_ep1i_set_or_clear_halt);
			}
		} else {
			// Unknown alternate setting.
			return USB_EP0_DISPOSITION_REJECT;
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static size_t flash_source_generate(void *UNUSED(opaque), void *buffer, size_t length) {
	if (read_bytes_left) {
		// There are some bytes left to read.
		// Compute how many we will read and read them into the buffer.
		size_t to_read = MIN(read_bytes_left, length);
		spi->read_bytes(buffer, to_read);

		// Mark that we have read that many bytes.
		read_bytes_left -= to_read;

		// If we have read all the bytes, the read is effectively no longer in progress.
		// Deassert /CS here, because if we don’t, we will not do it later on the next request because we will believe no read is in progress.
		if (!read_bytes_left) {
			spi->deassert_cs();
		}

		return to_read;
	} else {
		// We have reached the end of the Flash memory.
		// Indicate this to the host by way of a zero-byte 
		return 0;
	}
}

static void read_flash_poststatus(void) {
	// Complete the read operation.
	spi->deassert_cs();
	read_bytes_left = 0;
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *poststatus) {
	static uint8_t stash_buffer[3];
	static union {
		usb_ep0_memory_source_t mem_src;
		usb_ep0_source_t flash_src;
	} source_union;

	// Any request at all indicates that the previous request has ended.
	// If that request was a read, stop the read.
	stop_read();

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_JEDEC_ID) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request cannot be issued while monitoring in interrupt mode.
		if (interrupt_monitoring) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request uses the SPI bus.
		spi->drive_bus();

		// This request cannot be issued if the chip is already busy.
		if (read_status_register() & 1) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Issue the READ JEDEC ID command and store the three response bytes.
		spi->assert_cs();
		spi->transceive_byte(0x9F);
		spi->read_bytes(stash_buffer, 3);
		spi->deassert_cs();

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 3);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_READ_STATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request uses the SPI bus.
		spi->drive_bus();

		// Read the status register.
		stash_buffer[0] = read_status_register();

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_READ) {
		// This request must have value set to a legal page number.
		if (pkt->value >= NUM_PAGES) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request cannot be issued while monitoring in interrupt mode.
		if (interrupt_monitoring) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request uses the SPI bus.
		spi->drive_bus();

		// This request cannot be issued if the chip is already busy.
		if (read_status_register() & 1) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Compute how many bytes are available between the requested page number and the end of memory
		read_bytes_left = (NUM_PAGES - pkt->value) * PAGE_BYTES;

		// Issue the FAST READ instruction along with the address and dummy byte, but do not deassert /CS; it will stay asserted so the endpoint zero generator can read back data.
		spi->assert_cs();
		spi->transceive_byte(0x0B);
		spi->transceive_byte(pkt->value >> 8);
		spi->transceive_byte(pkt->value);
		spi->transceive_byte(0);
		spi->transceive_byte(0);

		source_union.flash_src.generate = &flash_source_generate;
		*source = &source_union.flash_src;
		*poststatus = &read_flash_poststatus;
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_INTERFACE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the alternate setting number.
		stash_buffer[0] = interrupt_mode ? 1 : 0;

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value set to zero.
		if (pkt->value != 0) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Interface status is always all zeroes.
		stash_buffer[0] = 0;
		stash_buffer[1] = 0;

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static bool write_postdata(void) {
	// Issue the WRITE ENABLE instruction.
	spi->assert_cs();
	spi->transceive_byte(0x06);
	spi->deassert_cs();

	// Issue the PAGE PROGRAM instruction with the address from the setup stage and the data from the data stage.
	spi->assert_cs();
	spi->transceive_byte(0x02);
	spi->transceive_byte(write_page >> 8);
	spi->transceive_byte(write_page);
	spi->transceive_byte(0);
	spi->write_bytes(write_buffer, PAGE_BYTES);
	spi->deassert_cs();

	// The chip must become busy as a result of the write starting.
	if (!(read_status_register() & 1)) {
		return false;
	}

	// If in interrupt mode, start monitoring the erase.
	if (interrupt_mode) {
		start_monitoring();
	}

	return true;
}

static usb_ep0_disposition_t on_out_request(const usb_ep0_setup_packet_t *pkt, void **dest, usb_ep0_postdata_cb_t *postdata, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	// Any request at all indicates that the previous request has ended.
	// If that request was a read, stop the read.
	stop_read();

	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_WRITE) {
		// This request must have value set to a legal page number.
		if (pkt->value >= NUM_PAGES) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request must have exactly PAGE_BYTES bytes of payload.
		if (pkt->length != PAGE_BYTES) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request cannot be issued while monitoring in interrupt mode.
		if (interrupt_monitoring) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request uses the SPI bus.
		spi->drive_bus();

		// This request cannot be issued if the chip is already busy.
		if (read_status_register() & 1) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Keep track of the page number to write to for later.
		write_page = pkt->value;

		// Provide a write buffer to hold the payload.
		*dest = write_buffer;
		*postdata = &write_postdata;
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = &on_out_request,
};

static bool can_enter(void) {
	return !autonomous_is_running();
}

static void on_enter_common(void) {
	// Initialize the variables.
	interrupt_mode = false;
	interrupt_monitoring = false;
	read_bytes_left = 0;

	// Turn on LED 2.
	gpio_set(GPIOB, 13);

	// Set up endpoint 1 IN with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(1, 64);
	usb_fifo_flush(1);

	// Register endpoints callbacks.
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_enter_target(void) {
	spi = &spi_external_ops;
	on_enter_common();
}

static void on_enter_onboard(void) {
	spi = &spi_internal_ops;
	on_enter_common();
}

static void on_exit_common(void) {
	// Stop monitoring, if we were.
	stop_monitoring();

	// Unregister endpoints callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);

	// Shut down the endpoint, if enabled.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Turn off LEDs 2 and 3.
	gpio_set_reset_mask(GPIOB, 0, 3 << 13);
}

static void on_exit_target(void) {
	on_exit_common();
	gpio_tristate_all();
}

static void on_exit_onboard(void) {
	on_exit_common();
}

const usb_configs_config_t TARGET_CONFIGURATION = {
	.configuration = 2,
	.can_enter = &can_enter,
	.on_enter = &on_enter_target,
	.on_exit = &on_exit_target,
};

const usb_configs_config_t ONBOARD_CONFIGURATION = {
	.configuration = 3,
	.can_enter = &can_enter,
	.on_enter = &on_enter_onboard,
	.on_exit = &on_exit_onboard,
};

