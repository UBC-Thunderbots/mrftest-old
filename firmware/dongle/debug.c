#include "debug.h"
#include "critsec.h"
#include "endpoints.h"
#include "local_error_queue.h"
#include "usb.h"
#include <pic18fregs.h>
#include <string.h>

/**
 * \brief The circular buffer into which characters are written.
 */
static char buffer[256];

/**
 * \brief The index in the circular buffer of the next character to write.
 */
static uint8_t write_ptr;

/**
 * \brief The index in the circular buffer of the next character to send over USB.
 */
static uint8_t send_ptr;

/**
 * \brief The index in the circular buffer of the earliest character submitted to the SIE.
 */
static uint8_t tail_ptr;

/**
 * \brief A bounce buffer for holding USB transactions that span the wraparound point in the circular buffer.
 */
static char bounce_buffer[64];

volatile BOOL debug_enabled = false;

/**
 * \brief Queues a block of bytes from the circular buffer for transmission over USB.
 *
 * \param[in] length the number of bytes to send (must be no greater than 64).
 */
static void queue_block(uint8_t length) {
	if ((uint8_t) (send_ptr + length) < send_ptr) {
		/* The block crosses the wraparound point. Use the bounce buffer. */
		memcpyram2ram(bounce_buffer, buffer + send_ptr, (uint8_t) -send_ptr);
		memcpyram2ram(bounce_buffer + (uint8_t) -send_ptr, buffer, length - (uint8_t) -send_ptr);
		USB_BD_IN_SUBMIT(EP_DEBUG, bounce_buffer, length);
	} else {
		/* The block does not cross the wraparound point. Use the buffer directly. */
		USB_BD_IN_SUBMIT(EP_DEBUG, buffer + send_ptr, length);
	}

	/* Update the pointer. */
	send_ptr += length;
}

static void check_send(void) {
	uint8_t pending, nl_offset;

	/* If there's no free BD, we have nothing to do. */
	if (!USB_BD_IN_HAS_FREE(EP_DEBUG)) {
		return;
	}

	/* If there are no bytes outstanding, we have nothing to do. */
	if (write_ptr == send_ptr) {
		return;
	}

	/* Compute number of bytes waiting to be sent, up to a maximum of 64. */
	pending = write_ptr - send_ptr;
	if (pending > 64) {
		pending = 64;
	}

	/* Search for a newline. */
	for (nl_offset = 0; nl_offset != pending; ++nl_offset) {
		if (buffer[(send_ptr + nl_offset) & 0xFF] == '\n') {
			break;
		}
	}

	/* Consider the cases. */
	if (nl_offset != pending) {
		/* We found a newline, meaning end of message, less than 64 bytes away.
		 * (Note, if it had been 64 bytes away, then nl_offset would have equalled pending)
		 * Send a short transaction (indicating end of transfer) with the remaining bytes in the message. */
		queue_block(nl_offset);
		/* We don't actually want to send the newline itself over USB, but we do want to advance the send pointer past it. */
		++send_ptr;
	} else if (pending == 64) {
		/* No newlines, but we do have 64 valid bytes to send, which is a full transaction.
		 * Send them. */
		queue_block(64);
	}
}

static void on_transaction(void) {
	/* Advance the tail pointer over the transmitted bytes. */
	tail_ptr += USB_BD_IN_SENT(EP_DEBUG);

	if (buffer[tail_ptr] == '\n' && USB_BD_IN_SENT(EP_DEBUG) != 64) {
		/* We just finished sending the last characters of a message.
		 * Also, the USB transaction was less than full length, so we properly signalled end of transfer to the host.
		 * Therefore, skip over the newline. */
		++tail_ptr;
	}

	/* See if we should send another transaction. */
	check_send();
}

static void on_commanded_stall(void) {
	if (debug_enabled) {
		USB_BD_IN_COMMANDED_STALL(EP_DEBUG);
	}
}

static BOOL on_clear_halt(void) {
	if (debug_enabled) {
		USB_BD_IN_UNSTALL(EP_DEBUG);
		write_ptr = send_ptr = tail_ptr = 0;
		return true;
	} else {
		return false;
	}
}

void debug_init(void) {
	debug_enabled = false;
	stdout = STREAM_USER;
}

void debug_enable(void) {
	write_ptr = send_ptr = tail_ptr = 0;
	USB_BD_IN_INIT(EP_DEBUG);
	usb_ep_callbacks[EP_DEBUG].in.transaction = &on_transaction;
	usb_ep_callbacks[EP_DEBUG].in.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_DEBUG].in.clear_halt = &on_clear_halt;
	UEPBITS(EP_DEBUG).EPHSHK = 1;
	UEPBITS(EP_DEBUG).EPINEN = 1;
	debug_enabled = true;
}

void debug_disable(void) {
	debug_enabled = false;
	UEPBITS(EP_DEBUG).EPINEN = 0;
}

PUTCHAR(ch) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (debug_enabled && !(usb_halted_in_endpoints & (1 << EP_DEBUG))) {
		if (((uint8_t) (write_ptr + 1)) != tail_ptr) {
			/* Add the byte to the circular buffer. */
			buffer[write_ptr] = ch;
			++write_ptr;

			/* Check if it's time to start a transaction. */
			check_send();
		} else {
			/* There is no space in the circular buffer.
			 * Report the error. */
			local_error_queue_add(39);
		}
	}

	CRITSEC_LEAVE(cs);
}

