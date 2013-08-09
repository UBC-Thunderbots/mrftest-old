#include "gpio.h"
#include "constants.h"
#include <registers.h>
#include <unused.h>
#include <usb_configs.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>

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
		*TARGET_IO_PIN_INFO[i].moder = (*TARGET_IO_PIN_INFO[i].moder & ~(3 << (TARGET_IO_PIN_INFO[i].bit * 2)));
	}
}

