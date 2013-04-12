#include "uart.h"
#include "autonomous.h"
#include "constants.h"
#include <deferred.h>
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

const uint8_t UART_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	USB_DTYPE_CONFIGURATION, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	4, // bConfigurationValue
	STRING_INDEX_CONFIG4, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	0, // bInterfaceSubClass
	0xFF, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	USB_DTYPE_ENDPOINT, // bDescriptorType
	0x81, // bEndpointAddress
	0x02, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	0, // bInterval
};

typedef enum {
	UART_EVENT_BYTES,
	UART_EVENT_ERRORS,
} uart_event_type_t;

typedef struct uart_event {
	struct uart_event *next;
	uart_event_type_t type;
	union {
		struct {
			size_t length;
			uint8_t buffer[256];
		} bytes;
		uint8_t errors;
	} data;
} uart_event_t;

static uart_event_t event_buffer[64];
static uart_event_t *events_free, *events_write, *events_read;
static bool usb_overrun_pending;

static uart_event_t *alloc_event(void) {
	uart_event_t *event = events_free;
	if (event) {
		events_free = event->next;
		event->next = 0;
	}
	return event;
}

static void free_event(uart_event_t *event) {
	event->next = events_free;
	events_free = event;
}

static void kick_events(void);

static void queue_event(uart_event_t *event) {
	event->next = 0;
	if (events_write) {
		events_write->next = event;
		events_write = event;
	} else {
		events_read = events_write = event;
	}
}

static uart_event_t *dequeue_event(void) {
	if (events_read) {
		uart_event_t *event = events_read;
		if (event->next) {
			events_read = event->next;
		} else {
			events_read = events_write = 0;
		}
		return event;
	} else {
		return 0;
	}
}

static void queue_byte(uint8_t byte) {
	if (events_write && events_write->type == UART_EVENT_BYTES && events_write->data.bytes.length < sizeof(events_write->data.bytes.buffer)) {
		events_write->data.bytes.buffer[events_write->data.bytes.length++] = byte;
	} else {
		uart_event_t *event = alloc_event();
		if (!event) {
			usb_overrun_pending = true;
			return;
		}
		event->type = UART_EVENT_BYTES;
		event->data.bytes.length = 1;
		event->data.bytes.buffer[0] = byte;
		queue_event(event);
	}
}

static void queue_errors(uint8_t errors) {
	if (events_write && events_write->type == UART_EVENT_ERRORS) {
		events_write->data.errors |= errors;
	} else {
		uart_event_t *event = alloc_event();
		if (!event) {
			usb_overrun_pending = true;
			return;
		}
		event->type = UART_EVENT_ERRORS;
		event->data.errors = errors;
		queue_event(event);
	}
}

static void kick_events(void) {
	// If the received message endpoint already has data queued, or it is halted, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more events.
	usb_bi_in_state_t state = usb_bi_in_get_state(1);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// If there are no received events waiting to be sent, do nothing.
	// We will get back here later when an MRF interrupt leads to a received message being queued and we can then push the message.
	if (!events_read) {
		return;
	}

	// If the first event is an error, rather than a bytes, set endpoint halt and let the host do a control request to sort it out.
	if (events_read->type == UART_EVENT_ERRORS) {
		usb_bi_in_halt(1);
		return;
	}

	// Pop an event from the queue
	uart_event_t *event = dequeue_event();

	// Start a transfer.
	// Received message transfers are variable length up to a known maximum size.
	usb_bi_in_start_transfer(1, event->data.bytes.length, 256, &kick_events, 0);

	// Push the data for this transfer.
	usb_bi_in_push(1, event->data.bytes.buffer, event->data.bytes.length);

	// Free the event buffer.
	free_event(event);

	// Check if a USB overrun occurred.
	if (usb_overrun_pending) {
		// Report this issue—we are now guaranteed to have at least one event buffer free.
		queue_errors(1 << 3);
		usb_overrun_pending = false;
	}
}

void usart1_interrupt_vector(void) {
	// Read the status register first, then the data register—reading both is required to clear many interrupt sources.
	uint32_t status = USART1_SR;
	uint8_t data = USART1_DR;

	// If there is a framing or noise error, it applies to the byte which we just read from the data register, so it must be queued for reporting first.
	if (status & (NF | FE)) {
		uint8_t errors = 0;
		if (status & NF) {
			errors |= 1 << 1;
		}
		if (status & FE) {
			errors |= 1 << 0;
		}
		queue_errors(errors);
	}

	// Queue the byte, if there is one, which there should always be except during spurious interrupts.
	if (status & USART_RXNE) {
		queue_byte(data);
	}

	// If there is an overrun error, it applies following the byte which we just read from the data register, so it must be queued for reporting now.
	if (status & ORE) {
		queue_errors(1 << 2);
	}

	// We may now be able to start a USB transfer.
	kick_events();
}

static bool check_ep1i_clear_halt(unsigned int UNUSED(ep)) {
	return !events_read || events_read->type == UART_EVENT_BYTES;
}

static void handle_ep1i_clear_halt(unsigned int UNUSED(ep)) {
	kick_events();
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[2];
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->request == CONTROL_REQUEST_GET_ERRORS && !pkt->index) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request must be issued when errors are pending.
		if (!events_read || events_read->type != UART_EVENT_ERRORS) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the errors and free the event.
		uart_event_t *event = dequeue_event();
		stash_buffer[0] = event->data.errors;
		free_event(event);
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE && !pkt->index) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the alternate setting number, which is always zero.
		stash_buffer[0] = 0;
		*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS && !pkt->index) {
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
	.on_in_request = &on_in_request,
};

static bool can_enter(void) {
	return !autonomous_is_running();
}

static void on_enter(void) {
	// Clear variables.
	events_read = events_write = 0;
	for (unsigned int i = 0; i < sizeof(event_buffer) / sizeof(*event_buffer) - 1; ++i) {
		event_buffer[i].next = &event_buffer[i + 1];
	}
	event_buffer[sizeof(event_buffer) / sizeof(*event_buffer) - 1].next = 0;
	events_free = &event_buffer[0];
	usb_overrun_pending = false;

	// Turn on LED 2.
	GPIOB_BSRR = GPIO_BS(13);

	// Put a pull-up resistor on PB7 (UART receive).
	GPIOB_PUPDR |= PUPDR(7, 1);

	// Set up endpoint 1 IN with a 512-byte FIFO, large enough to hold any transfer.
	usb_fifo_enable(1, 512);
	usb_fifo_flush(1);
	usb_bi_in_init(1, 64, USB_BI_IN_EP_TYPE_BULK);
	usb_bi_in_set_std_halt(1, 0, &check_ep1i_clear_halt, &handle_ep1i_clear_halt);

	// Register endpoints callbacks.
	usb_ep0_cbs_push(&EP0_CBS);

	// Enable and configure USART 1 receiver.
	// Baud rate calculation:
	// APB2 clock is 84 MHz.
	// We want 9600 bps.
	// Equation for baud rate from datasheet: baud = fCK / (8 × (2 - OVER8) × USARTDIV)
	// 9600 = 84e6 / (8 × (2 - 0) × USARTDIV)
	// USARTDIV = 546.875
	// Mantissa part is 546
	// Fractional part, since OVER8 = 0, is 0.875 × 16 = 14
	rcc_enable(APB2, 4);
	USART1_CR1 = UE;
	USART1_CR2 = STOP(0) /* One stop bit */;
	USART1_CR3 = 0;
	USART1_BRR = DIV_Mantissa(546) | DIV_Fraction(14);
	USART1_CR1 |= USART_RXNEIE | RE; // Enable receiver and receiver interrupts
	(void) USART1_SR; // Read the status register to prepare to clear any pending error sources, etc.
	(void) USART1_DR; // Read the data register to actually clear any pending error sources, etc.
	NVIC_ICPR[37 / 32] = 1 << (37 % 32); // CLRPEND37 = 1; clear pending USART1 interrupts
	NVIC_ISER[37 / 32] = 1 << (37 % 32); // SETENA37 = 1; enable USART1 interrupts
}

static void on_exit(void) {
	// Disable USART 1 receiver.
	NVIC_ICER[37 / 32] = 1 << (37 % 32); // CLRENA37 = 1; disable USART1 interrupts
	USART1_CR1 = 0;
	rcc_disable(APB2, 4);

	// Unregister endpoints callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);

	// Shut down the endpoint.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Remove the pull-up resistor from PB7 (UART receive).
	GPIOB_PUPDR &= ~PUPDR(7, 3);

	// Turn off LEDs 2 and 3.
	GPIOB_BSRR = GPIO_BR(13) | GPIO_BR(14);
}

const usb_configs_config_t UART_CONFIGURATION = {
	.configuration = 4,
	.can_enter = &can_enter,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

