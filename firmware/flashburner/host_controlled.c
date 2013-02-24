#include "host_controlled.h"
#include "constants.h"
#include "spi.h"
#include <minmax.h>
#include <rcc.h>
#include <registers.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unused.h>
#include <usb_bi_in.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>

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
	0, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	1, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	0, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
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
	0, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	1, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	0, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
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

static uint8_t read_status_register(void) {
	spi->assert_cs();
	spi->transceive_byte(0x05);
	uint8_t status = spi->transceive_byte(0);
	spi->deassert_cs();
	return status;
}

static void start_monitoring(void) {
	interrupt_monitoring = true;
	TIM6_CR1 |= CEN;
	NVIC_ISER[54 / 32] = 1 << (54 % 32); // SETENA54 = 1; enable timer 6 interrupt
}

static void stop_monitoring(void) {
	interrupt_monitoring = false;
	TIM6_CR1 &= ~CEN;
	NVIC_ICER[54 / 32] = 1 << (54 % 32); // CLRENA54 = 1; disable timer 6 interrupt
}

void timer6_interrupt_vector(void) {
	// Clear interrupt flag.
	TIM6_SR = 0;

	if (interrupt_monitoring) {
		// A write or erase has been requested and the module is set up to autopoll and report completion on interrupt IN endpoint 1.
		// Poll the status now.
		uint8_t status = read_status_register();
		if (!(status & 1)) {
			// The write has completed.
			// Stop monitoring now.
			stop_monitoring();

			// Start a transfer on the endpoint to notify the host.
			if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_IDLE) {
				usb_bi_in_start_transfer(1, 0, 1, 0, 0);
			}
		}
	} else {
		// No write or erase currently needs polling; disable the timer.
		stop_monitoring();
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

typedef struct {
	volatile uint32_t *idr;
	volatile uint32_t *odr;
	volatile uint32_t *moder;
	unsigned int bit;
} target_io_pin_info_t;

static const target_io_pin_info_t TARGET_IO_PIN_INFO[] = {
	// PB5 = external Flash MOSI
	{ &GPIOB_IDR, &GPIOB_ODR, &GPIOB_MODER, 5 },
	// PB4 = external Flash MISO
	{ &GPIOB_IDR, &GPIOB_ODR, &GPIOB_MODER, 4 },
	// PB3 = external Flash SCK
	{ &GPIOB_IDR, &GPIOB_ODR, &GPIOB_MODER, 3 },
	// PA15 = external Flash /CS
	{ &GPIOA_IDR, &GPIOA_ODR, &GPIOA_MODER, 15 },
	// PD2 = external power control
	{ &GPIOD_IDR, &GPIOD_ODR, &GPIOD_MODER, 2 },
	// PA14 = PROGRAM_B
	{ &GPIOA_IDR, &GPIOA_ODR, &GPIOA_MODER, 14 },
};

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	// Any request at all indicates that the previous request has ended.
	// If that request was a read, stop the read.
	stop_read();

	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_WRITE_IO) {
		// Only the bottom 6 bits of each byte are defined; the others must be zero.
		if (pkt->value != (pkt->value & 0x3F3F)) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request is only legal in configuration 2.
		// Configuration 3 accesses the onboard Flash memory, so the target connector is not used.
		if (usb_configs_get_current() != 2) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request cannot be issued while monitoring in interrupt mode.
		if (interrupt_monitoring) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// LSB indicates drive level.
		// MSB indicates direction.
		// Apply the requested states.
		for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
			if (pkt->value & (1 << (i + 8))) {
				if (pkt->value & (1 << i)) {
					*TARGET_IO_PIN_INFO[i].odr |= 1 << TARGET_IO_PIN_INFO[i].bit;
				} else {
					*TARGET_IO_PIN_INFO[i].odr &= ~(1 << TARGET_IO_PIN_INFO[i].bit);
				}
				*TARGET_IO_PIN_INFO[i].moder = (*TARGET_IO_PIN_INFO[i].moder & ~(3 << (TARGET_IO_PIN_INFO[i].bit * 2))) | (1 << (TARGET_IO_PIN_INFO[i].bit * 2));
			} else {
				*TARGET_IO_PIN_INFO[i].moder = (*TARGET_IO_PIN_INFO[i].moder & ~(3 << (TARGET_IO_PIN_INFO[i].bit * 2)));
			}
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_ERASE && !pkt->index) {
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

		// Issue the WRITE ENABLE instruction.
		spi->assert_cs();
		spi->transceive_byte(0x06);
		spi->deassert_cs();

		// Issue the CHIP ERASE instruction.
		spi->assert_cs();
		spi->transceive_byte(0xC7);
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
	} else if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_SET_INTERFACE && !pkt->index) {
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

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ_IO && !pkt->index) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request is only legal in configuration 2.
		// Configuration 3 accesses the onboard Flash memory, so the target connector is not used.
		if (usb_configs_get_current() != 2) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Clear the buffer.
		stash_buffer[0] = stash_buffer[1] = 0;

		// First byte indicates pin level.
		// Second byte indicates 1=driven, 0=tristated.
		// Read out the current states.
		for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
			unsigned int mode = ((*TARGET_IO_PIN_INFO[i].moder) >> (TARGET_IO_PIN_INFO[i].bit * 2)) & 3;
			if (mode == 0) {
				// This pin is a GP input.
				if ((*TARGET_IO_PIN_INFO[i].idr) & (1 << TARGET_IO_PIN_INFO[i].bit)) {
					stash_buffer[0] |= 1 << i;
				}
			} else if (mode == 1) {
				// This pin is a GP output.
				if ((*TARGET_IO_PIN_INFO[i].odr) & (1 << TARGET_IO_PIN_INFO[i].bit)) {
					stash_buffer[0] |= 1 << i;
				}
				stash_buffer[1] |= 1 << i;
			} else if (mode == 2) {
				// This pin is an alternate function.
			}
		}

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_JEDEC_ID && !pkt->index) {
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
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ_STATUS && !pkt->index) {
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
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ && !pkt->index) {
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
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE && !pkt->index) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the alternate setting number.
		stash_buffer[0] = interrupt_mode ? 1 : 0;

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS && !pkt->index) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Interface status is always all zeroes.
		stash_buffer[0] = 0;
		stash_buffer[1] = 0;

		*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
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

	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_WRITE && !pkt->index) {
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
#warning TODO fail this request if autonomous mode running
	return true;
}

static void on_enter_common(void) {
	// Initialize the variables.
	interrupt_mode = false;
	interrupt_monitoring = false;
	read_bytes_left = 0;

	// Turn on LED 2.
	GPIOB_BSRR = GPIO_BS(13);

	// Set up endpoint 1 IN with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(1, 64);
	usb_fifo_flush(1);

	// Set up timer 6 to overflow every 250 microseconds, but do not start it yet.
	// Timer 6 input is 84 MHz from the APB.
	// Need to count to 21,000 for each overflow.
	// Set prescaler to 1, auto-reload to 21,000.
	rcc_enable(APB1, 4);
	TIM6_CR1 = 0 // Auto reload not buffered, counter runs continuously (not just for one pulse), updates not disabled, counter disabled for now
		| URS; // Only overflow generates an interrupt
	TIM6_DIER = UIE; // Update interrupt enabled
	TIM6_PSC = 0;
	TIM6_ARR = 20999;
	TIM6_CNT = 0;

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

	// Power down timer 6.
	rcc_disable(APB1, 4);

	// Shut down the endpoint, if enabled.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Turn off LEDs 2 and 3.
	GPIOB_BSRR = GPIO_BR(13) | GPIO_BR(14);
}

static void on_exit_target(void) {
	on_exit_common();

	// Tristate all I/O pins.
	for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
		*TARGET_IO_PIN_INFO[i].moder = (*TARGET_IO_PIN_INFO[i].moder & ~(3 << (TARGET_IO_PIN_INFO[i].bit * 2)));
	}
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

