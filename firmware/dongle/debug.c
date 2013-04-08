#include "debug.h"
#include "constants.h"
#include "mrf.h"
#include <exti.h>
#include <rcc.h>
#include <registers.h>
#include <stdint.h>
#include <unused.h>
#include <usb_bi_in.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>

const uint8_t DEBUG_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	2, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	6, // bConfigurationValue
	8, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	5, // bDescriptorType
	0x81, // bEndpointAddress
	0x03, // bmAttributes
	1, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	10, // bInterval
};

static uint64_t int_buffer;
static size_t int_buffer_used;

static void push_int_notify(void) {
	// If the interrupt notification endpoint already has data queued, or it is halted, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more notifications.
	usb_bi_in_state_t state = usb_bi_in_get_state(1);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// If there are no interrupt notifications waiting to be sent, do nothing.
	// We will get back here later when pin change leads to a notification being queued and we can then push the message.
	if (!int_buffer_used) {
		return;
	}

	// Pop a notification from the queue.
	bool value = !!(int_buffer & 1);
	int_buffer >>= 1;
	--int_buffer_used;

	// Start a transfer.
	// Interrupt notification transfers are always one byte long.
	usb_bi_in_start_transfer(1, 1, 1, &push_int_notify, 0);

	// Push the data for this transfer.
	usb_bi_in_push(1, &value, 1);
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt.
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	// Check the INT pin level.
	bool int_level = !!(GPIOC_IDR & (1 << 12));

	// Display the INT pin level on LED 2.
	GPIOB_BSRR = int_level ? GPIO_BS(13) : GPIO_BR(13);

	if (usb_bi_in_get_state(1) != USB_BI_IN_STATE_HALTED) {
		// Buffer the new state.
		if (int_buffer_used == 64) {
			int_buffer_used = 63;
		}
		int_buffer = (int_buffer & ~(UINT64_C(1) << 63)) | ((int_level ? 1 : 0) << int_buffer_used);
		++int_buffer_used;

		// Try to push an interrupt notification.
		push_int_notify();
	}
}

static void handle_ep1_clear_halt(unsigned int UNUSED(ep)) {
	usb_bi_in_reset_pid(1);
	int_buffer_used = 0;
}

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_CONTROL_LINES) {
		// This request must have only the bottom two bits of value set nonzero and must have index set to zero.
		if (pkt->value & 0b1111111111111100 || pkt->index) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Set the pins appropriately.
		GPIOB_BSRR = (pkt->value & (1 << 0)) ? GPIO_BS(7) : GPIO_BR(7);
		GPIOB_BSRR = (pkt->value & (1 << 1)) ? GPIO_BS(6) : GPIO_BR(6);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_SHORT_REGISTER) {
		// This request must have value set to a valid byte and index set to a valid short register address.
		if (pkt->value > 0xFF || pkt->index > 0x3F) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Write to the register.
		mrf_write_short(pkt->index, pkt->value);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_LONG_REGISTER) {
		// This request must have value set to a valid byte and index set to a valid long register address.
		if (pkt->value > 0xFF || pkt->index > 0x038F || (0x02C0 <= pkt->index && pkt->index <= 0x02FF)) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Write to the register.
		mrf_write_long(pkt->index, pkt->value);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[2];
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_CONTROL_LINES) {
		// This request must have value and index set to zero.
		if (pkt->value || pkt->index) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Read the control lines.
		stash_buffer[0] = 0;
		if (GPIOB_ODR & (1 << 7)) {
			stash_buffer[0] |= 1 << 0;
		}
		if (GPIOB_ODR & (1 << 6)) {
			stash_buffer[0] |= 1 << 1;
		}
		if (mrf_get_interrupt()) {
			stash_buffer[0] |= 1 << 2;
		}

		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_SHORT_REGISTER) {
		// This request must have value set to zero and index set to a valid short register address.
		if (pkt->value || pkt->index > 0x3F) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Read the register.
		stash_buffer[0] = mrf_read_short(pkt->index);
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_LONG_REGISTER) {
		// This request must have value set to zero and index set to a valid long register address.
		if (pkt->value || pkt->index > 0x038F || (0x02C0 <= pkt->index && pkt->index <= 0x02FF)) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Read the register.
		stash_buffer[0] = mrf_read_long(pkt->index);
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_INTERFACE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the alternate setting number, which is always zero.
		stash_buffer[0] = 0;
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Interface status is always all zeroes.
		stash_buffer[0] = 0;
		stash_buffer[1] = 0;
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

static void on_enter(void) {
	// Initialize radio.
	mrf_init();

	// Turn on LED 1.
	GPIOB_BSRR = GPIO_BS(12);

	// Configure MRF INT (PC12) as an external interrupt.
	exti_set_handler(12, &exti12_interrupt_vector);
	exti_map(12, 2); // Map PC12 to EXTI12
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR |= 1 << 12; // TR12 = 1; enable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Clear the interrupt buffer.
	int_buffer_used = 0;

	// Set up IN endpoint 1 with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(1, 64);
	usb_fifo_flush(1);
	usb_bi_in_init(1, 1, USB_BI_IN_EP_TYPE_INTERRUPT);
	usb_bi_in_set_std_halt(1, 0, 0, &handle_ep1_clear_halt);

	// Display the current level of INT on LED 2.
	bool int_level = !!(GPIOC_IDR & (1 << 12));
	GPIOB_BSRR = int_level ? GPIO_BS(13) : GPIO_BR(13);

	// Register endpoints callbacks.
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	// Unregister endpoints callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);

	// Shut down IN endpoint 1.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Disable the external interrupt on MRF INT.
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	exti_set_handler(12, 0);

	// Turn off all LEDs.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Reset the radio.
	mrf_init();
}

const usb_configs_config_t DEBUG_CONFIGURATION = {
	.configuration = 6,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

