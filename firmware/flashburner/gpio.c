#include "gpio.h"
#include "constants.h"
#include <gpio.h>
#include <unused.h>
#include <usb_configs.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>

typedef struct {
	volatile GPIO_t *gpio;
	unsigned int bit;
} target_io_pin_info_t;

static const target_io_pin_info_t TARGET_IO_PIN_INFO[] = {
	// PB5 = external Flash MOSI
	{ &GPIOB, 5 },
	// PB4 = external Flash MISO
	{ &GPIOB, 4 },
	// PB3 = external Flash SCK
	{ &GPIOB, 3 },
	// PA15 = external Flash /CS
	{ &GPIOA, 15 },
	// PD2 = external power control
	{ &GPIOD, 2 },
	// PA14 = PROGRAM_B
	{ &GPIOA, 14 },
};

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_WRITE_IO) {
		// Only the bottom 6 bits of each byte are defined; the others must be zero.
		if (pkt->value != (pkt->value & 0x3F3F)) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request only works in host-controlled and UART mode.
		if (usb_configs_get_current() < 2) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// LSB indicates drive level.
		// MSB indicates direction.
		// Apply the requested states.
		for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
			if (pkt->value & (1 << (i + 8))) {
				unsigned int mask = 1 << TARGET_IO_PIN_INFO[i].bit;
				gpio_set_reset_mask(*TARGET_IO_PIN_INFO[i].gpio, (pkt->value & (1 << i)) ? mask : 0, mask);
				gpio_set_mode(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit, GPIO_MODE_OUT);
			} else {
				gpio_set_mode(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit, GPIO_MODE_IN);
			}
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[2];
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == CONTROL_REQUEST_READ_IO) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request only works in host-controlled and UART mode.
		if (usb_configs_get_current() < 2) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Clear the buffer.
		stash_buffer[0] = stash_buffer[1] = 0;

		// First byte indicates pin level.
		// Second byte indicates 1=driven, 0=tristated.
		// Read out the current states.
		for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
			GPIO_MODE_t mode = gpio_get_mode(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit);
			if (mode == GPIO_MODE_IN) {
				// This pin is a GP input.
				if (gpio_get_input(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit)) {
					stash_buffer[0] |= 1 << i;
				}
			} else if (mode == GPIO_MODE_OUT) {
				// This pin is a GP output.
				if (gpio_get_output(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit)) {
					stash_buffer[0] |= 1 << i;
				}
				stash_buffer[1] |= 1 << i;
			} else if (mode == GPIO_MODE_AF) {
				// This pin is an alternate function.
			}
		}

		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = 0,
};

void gpio_push_ep0_cbs(void) {
	usb_ep0_cbs_push(&EP0_CBS);
}

void gpio_remove_ep0_cbs(void) {
	usb_ep0_cbs_remove(&EP0_CBS);
}

void gpio_tristate_all(void) {
	for (unsigned int i = 0; i < sizeof(TARGET_IO_PIN_INFO) / sizeof(*TARGET_IO_PIN_INFO); ++i) {
		gpio_set_mode(*TARGET_IO_PIN_INFO[i].gpio, TARGET_IO_PIN_INFO[i].bit, GPIO_MODE_IN);
	}
}

