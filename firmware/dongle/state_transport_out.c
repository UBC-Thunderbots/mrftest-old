#include "state_transport_out.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "local_error_queue.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

__data uint8_t state_transport_out_drive[15][STATE_TRANSPORT_OUT_DRIVE_SIZE];

/**
 * \brief A buffer into which packets are received.
 */
static uint8_t buffer[64];

static void on_transaction(void) {
	uint8_t recipient;
	__data const uint8_t *ptr = buffer;
	uint8_t left = USB_BD_OUT_RECEIVED(EP_STATE_TRANSPORT);

	/* Decode the packet. */
	while (left) {
		if (ptr[0] <= left) {
			switch (ptr[1] & 0x0F) {
				case 0:
					/* Drive pipe. */
					if (ptr[0] == STATE_TRANSPORT_OUT_DRIVE_SIZE + 2) {
						recipient = ptr[1] >> 4;
						if (recipient) {
							/* Directed. */
							memcpyram2ram(state_transport_out_drive[recipient - 1], ptr + 2, STATE_TRANSPORT_OUT_DRIVE_SIZE);
						} else {
							/* Broadcast. */
							for (recipient = 0; recipient != 15; ++recipient) {
								memcpyram2ram(state_transport_out_drive[recipient], ptr + 2, STATE_TRANSPORT_OUT_DRIVE_SIZE);
							}
						}
					} else {
						local_error_queue_add(37);
					}
					break;

				default:
					/* No such pipe. */
					local_error_queue_add(38);
					break;
			}
			left -= ptr[0];
			ptr += ptr[0];
		} else {
			local_error_queue_add(37);
			break;
		}
	}

	/* Prepare to receive the next packet. */
	USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
}

static void on_commanded_stall(void) {
	USB_BD_OUT_COMMANDED_STALL(EP_STATE_TRANSPORT);
}

static BOOL on_clear_halt(void) {
	/* Halt status can only be cleared once XBee stage 2 configuration completes. */
	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		USB_BD_OUT_UNSTALL(EP_STATE_TRANSPORT);
		USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
		return true;
	} else {
		return false;
	}
}

void state_transport_out_init(void) {
	/* The endpoint is halted until XBee stage 2 configuration completes. */
	usb_halted_out_endpoints |= 1 << EP_STATE_TRANSPORT;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.transaction = &on_transaction;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.clear_halt = &on_clear_halt;
	USB_BD_OUT_INIT(EP_STATE_TRANSPORT);
	USB_BD_OUT_FUNCTIONAL_STALL(EP_STATE_TRANSPORT);
	UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 1;
}

void state_transport_out_deinit(void) {
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 0;
}

