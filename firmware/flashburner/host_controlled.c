#include "host_controlled.h"
#include "constants.h"
#include "spi.h"
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

#define NUM_PAGES (2 * 1024 * 1024 / 256)

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

static volatile bool read_in_progress = false, autopolled_write_in_progress = false, autopolled_mode = false;
static const struct spi_ops *spi = 0;
static uint16_t write_page;
static uint8_t write_buffer[256];

static uint8_t read_status_register(void) {
	spi->enable();
	spi->assert_cs();
	spi->transceive_byte(0x05);
	uint8_t status = spi->transceive_byte(0);
	spi->deassert_cs();
	spi->disable();
	return status;
}

static void start_timer6(void) {
	TIM6_CR1 |= CEN;
	NVIC_ISER[54 / 32] = 1 << (54 % 32); // SETENA54 = 1; enable timer 6 interrupt
}

static void stop_timer6(void) {
	TIM6_CR1 &= ~CEN;
	NVIC_ICER[54 / 32] = 1 << (54 % 32); // CLRENA54 = 1; disable timer 6 interrupt
}

void timer6_interrupt_vector(void) {
	// Clear interrupt flag.
	TIM6_SR = 0;

	if (autopolled_write_in_progress) {
		// A write or erase has been requested and the module is set up to autopoll and report completion on interrupt IN endpoint 1.
		// Poll the status now.
		uint8_t status = read_status_register();
		if (!(status & 1)) {
			// The write has completed.
			// Account.
			autopolled_write_in_progress = false;

			// Disable this timer.
			stop_timer6();

			// Start a transfer on the endpoint to notify the host.
			usb_bi_in_start_transfer(1, 0, 1, 0, 0);
		}
	} else {
		// No write or erase currently needs polling; disable the timer.
		stop_timer6();
	}
}

static void handle_ep1i_set_or_clear_halt(unsigned int UNUSED(ep)) {
	// Stop polling for any currently-running write.
	autopolled_write_in_progress = false;
	stop_timer6();
}

static void stop_background(void) {
	// Stop any in-progress operation (writes and erases can’t actually be stopped, but we can set the halt feature on the reporting endpoint if enabled and stop polling).
	if (read_in_progress) {
		spi->deassert_cs();
		spi->disable();
		read_in_progress = false;
	} else if (autopolled_write_in_progress) {
		TIM6_CR1 &= ~CEN;
		stop_timer6();
		if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_ACTIVE) {
			usb_bi_in_abort_transfer(1);
		}
		if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_IDLE) {
			usb_bi_in_halt(1);
		}
		autopolled_write_in_progress = false;
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
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_WRITE_IO) {
		if (!pkt->index && usb_configs_get_current() == 2) {
			// LSB indicates drive level.
			// MSB indicates direction.
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
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_ERASE) {
		if (!pkt->value && !pkt->index) {
			stop_background();
			spi->enable();
			spi->assert_cs();
			spi->transceive_byte(0x06);
			spi->deassert_cs();
			spi->assert_cs();
			spi->transceive_byte(0xC7);
			spi->deassert_cs();
			spi->disable();
			if (autopolled_mode) {
				autopolled_write_in_progress = true;
				start_timer6();
			}
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_SET_INTERFACE) {
		if (pkt->value <= 1 && !pkt->index) {
			// Kill any background operation.
			stop_background();

			if (pkt->value == 0) {
				// Switching to non-autopolled mode.
				autopolled_mode = false;
				usb_bi_in_deinit(1);
			} else {
				// Switching to autopolled mode.
				autopolled_mode = true;
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
			}

			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static size_t flash_source_generate(void *UNUSED(opaque), void *buffer, size_t length) {
	if (read_in_progress) {
		spi->read_bytes(buffer, length);
		return length;
	} else {
		return 0;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[3];
	static union {
		usb_ep0_memory_source_t mem_src;
		usb_ep0_source_t flash_src;
	} source_union;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ_IO) {
		if (!pkt->value && !pkt->index && usb_configs_get_current() == 2) {
			// Clear the buffer.
			stash_buffer[0] = stash_buffer[1] = 0;
			// First byte indicates pin level.
			// Second byte indicates 1=driven, 0=tristated.
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
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_JEDEC_ID) {
		if (!pkt->value && !pkt->index) {
			stop_background();
			spi->enable();
			spi->assert_cs();
			spi->transceive_byte(0x9F);
			spi->read_bytes(stash_buffer, 3);
			spi->deassert_cs();
			spi->disable();
			*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 3);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ_STATUS) {
		if (!pkt->value && !pkt->index) {
			if (read_in_progress) {
				stop_background();
			}
			stash_buffer[0] = read_status_register();
			*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_READ) {
		if (pkt->value + (pkt->length / 256) <= NUM_PAGES && !pkt->index) {
			stop_background();
			spi->enable();
			spi->assert_cs();
			spi->transceive_byte(0x0B);
			spi->transceive_byte(pkt->value >> 8);
			spi->transceive_byte(pkt->value);
			spi->transceive_byte(0);
			spi->transceive_byte(0);
			read_in_progress = true;
			source_union.flash_src.generate = &flash_source_generate;
			*source = &source_union.flash_src;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE) {
		if (!pkt->value && !pkt->index && pkt->length == 1) {
			stash_buffer[0] = autopolled_mode ? 1 : 0;
			*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS) {
		if (!pkt->value && !pkt->index && pkt->length == 2) {
			stash_buffer[0] = 0;
			stash_buffer[1] = 0;
			*source = usb_ep0_memory_source_init(&source_union.mem_src, stash_buffer, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static bool write_postdata(void) {
	spi->enable();
	spi->assert_cs();
	spi->transceive_byte(0x06);
	spi->deassert_cs();
	spi->assert_cs();
	spi->transceive_byte(0x02);
	spi->transceive_byte(write_page >> 8);
	spi->transceive_byte(write_page);
	spi->transceive_byte(0);
	spi->write_bytes(write_buffer, 256);
	spi->deassert_cs();
	spi->disable();
	if (autopolled_mode) {
		autopolled_write_in_progress = true;
		start_timer6();
	}
	return true;
}

static usb_ep0_disposition_t on_out_request(const usb_ep0_setup_packet_t *pkt, void **dest, usb_ep0_postdata_cb_t *postdata, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_WRITE) {
		if (pkt->value < NUM_PAGES && pkt->length == 256) {
			stop_background();
			write_page = pkt->value;
			*dest = write_buffer;
			*postdata = &write_postdata;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = &on_out_request,
};

static bool can_enter_target(void) {
#warning TODO fail this request if autonomous mode running
	return true;
}

static bool can_enter_onboard(void) {
#warning TODO fail this request if autonomous mode running
	spi_internal_ops.enable();
	spi_internal_ops.assert_cs();
	spi_internal_ops.transceive_byte(0x9F);
	uint8_t mfgr = spi_internal_ops.transceive_byte(0);
	uint8_t type = spi_internal_ops.transceive_byte(0);
	uint8_t capacity = spi_internal_ops.transceive_byte(0);
	spi_internal_ops.deassert_cs();
	spi_internal_ops.disable();
	return mfgr == 0xEF && type == 0x40 && capacity == 0x15;
}

static void on_enter_common(void) {
	// Initialize the variables.
	read_in_progress = autopolled_write_in_progress = autopolled_mode = false;

	// Turn on LED 1.
	GPIOB_BSRR = GPIO_BS(12);

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
	GPIOB_BSRR = GPIO_BS(13);
	spi = &spi_external_ops;
	on_enter_common();
}

static void on_enter_onboard(void) {
	GPIOB_BSRR = GPIO_BS(14);
	spi = &spi_internal_ops;
	on_enter_common();
}

static void on_exit_common(void) {
	// Stop any background operation.
	stop_background();

	// Unregister endpoints callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);

	// Turn off timer 6.
	TIM6_CR1 = 0; // Disable counter
	NVIC_ICER[54 / 32] = 1 << (54 % 32); // CLRENA54 = 1; disable timer 6 interrupt
	rcc_disable(APB1, 4);

	// Shut down the endpoint, if enabled.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Turn off all LEDs.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);
}

static void on_exit_target(void) {
	on_exit_common();

	// Tristate all the I/O pins.
	for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
		*TARGET_IO_PIN_INFO[i].moder = (*TARGET_IO_PIN_INFO[i].moder & ~(3 << (TARGET_IO_PIN_INFO[i].bit * 2)));
	}
}

static void on_exit_onboard(void) {
	on_exit_common();
}

const usb_configs_config_t TARGET_CONFIGURATION = {
	.configuration = 2,
	.can_enter = &can_enter_target,
	.on_enter = &on_enter_target,
	.on_exit = &on_exit_target,
};

const usb_configs_config_t ONBOARD_CONFIGURATION = {
	.configuration = 3,
	.can_enter = &can_enter_onboard,
	.on_enter = &on_enter_onboard,
	.on_exit = &on_exit_onboard,
};

