#include "configs.h"
#include "constants.h"
#include "exti.h"
#include "interrupt.h"
#include "mrf.h"
#include "rcc.h"
#include "registers.h"
#include "stdint.h"
#include "usb.h"
#include "usb_bi_in.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"
#include "usb_fifo.h"

const uint8_t CONFIGURATION_DESCRIPTOR6[] = {
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
	1, // bInterval
};

static uint64_t int_buffer;
static size_t int_buffer_used;

static void push_int_notify(void) {
	// If the interrupt notification endpoint already has data queued, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more notifications.
	if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_ACTIVE) {
		return;
	}

	// If there are no interrupt notifications waiting to be sent, do nothing.
	// We will get back here later when pin change leads to a notification being queued and we can then push the message.
	if (!int_buffer_used) {
		return;
	}

	// Pop a notification from the queue
	bool value = !!(int_buffer & 1);
	int_buffer >>= 1;
	--int_buffer_used;

	// Start a transfer.
	// Interrupt notification transfers are always one byte long.
	usb_bi_in_start_transfer(1, 1, 1, &push_int_notify, 0);

	// Push the data for this transfer.
	usb_bi_in_push_word(1, value);
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt.
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	// Check the INT pin level.
	bool int_level = !!(GPIOC_IDR & (1 << 12));

	// Display the INT pin level on LED 2.
	GPIOB_BSRR = int_level ? GPIO_BS(13) : GPIO_BR(13);

	// Buffer the new state.
	if (int_buffer_used == 64) {
		int_buffer_used = 63;
	}
	int_buffer = (int_buffer & ~(UINT64_C(1) << 63)) | ((int_level ? 1 : 0) << int_buffer_used);
	++int_buffer_used;

	// Try to push an interrupt notification.
	push_int_notify();
}

static void on_enter(void) {
	// Initialize radio
	mrf_init();

	// Turn on LED 1
	GPIOB_BSRR = GPIO_BS(12);

	// Configure MRF INT (PC12) as an external interrupt
	interrupt_exti12_handler = &exti12_interrupt_vector;
	rcc_enable(APB2, 14);
	exti_map(12, 2); // Map PC12 to EXTI12
	rcc_disable(APB2, 14);
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR |= 1 << 12; // TR12 = 1; enable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Clear the interrupt buffer.
	int_buffer_used = 0;

	// Set up IN endpoint 1 with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_set_size(1, 64);
	usb_fifo_flush(1);
	usb_bi_in_init(1, 1, USB_BI_IN_EP_TYPE_INTERRUPT);

	// Display the current level of INT on LED 2.
	bool int_level = !!(GPIOC_IDR & (1 << 12));
	GPIOB_BSRR = int_level ? GPIO_BS(13) : GPIO_BR(13);
}

static void on_exit(void) {
	// Shut down IN endpoint 1.
	if (usb_bi_in_get_state(1) == USB_BI_IN_STATE_ACTIVE) {
		usb_bi_in_abort_transfer(1);
		usb_fifo_flush(1);
	}
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_reset();

	// Disable the external interrupt on MRF INT.
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	interrupt_exti12_handler = 0;

	// Turn off all LEDs.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Reset the radio.
	mrf_init();
}

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept) {
	if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_CONTROL_LINES && !(value & 0b1111111111111100) && !index) {
		GPIOB_BSRR = (value & (1 << 0)) ? GPIO_BS(7) : GPIO_BR(7);
		GPIOB_BSRR = (value & (1 << 1)) ? GPIO_BS(6) : GPIO_BR(6);
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_SHORT_REGISTER && value <= 0xFF && index <= 0x3F) {
		mrf_write_short(index, value);
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_LONG_REGISTER && value <= 0xFF && index <= 0x038F && !(0x02C0 <= index && index <= 0x02FF)) {
		mrf_write_long(index, value);
		*accept = true;
		return true;
	} else {
		return false;
	}
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length __attribute__((unused)), usb_ep0_source_t **source) {
	static uint8_t buffer[1];
	static usb_ep0_memory_source_t mem_src;

	if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_CONTROL_LINES && !value && !index) {
		buffer[0] = 0;
		if (GPIOB_ODR & (1 << 7)) {
			buffer[0] |= 1 << 0;
		}
		if (GPIOB_ODR & (1 << 6)) {
			buffer[0] |= 1 << 1;
		}
		if (mrf_get_interrupt()) {
			buffer[0] |= 1 << 2;
		}
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_SHORT_REGISTER && !value && index <= 0x3F) {
		buffer[0] = mrf_read_short(index);
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_LONG_REGISTER && !value && index <= 0x038F && !(0x02C0 <= index && index <= 0x02FF)) {
		buffer[0] = mrf_read_long(index);
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else {
		return false;
	}
}

const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS6 = {
	.configuration = 6,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 1,
	.can_enter = 0,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = 0,
};

