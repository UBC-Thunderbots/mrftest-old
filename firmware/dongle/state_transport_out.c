#include "state_transport_out.h"
#include "endpoints.h"
#include "local_error_queue.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

__data uint8_t state_transport_out_drive[15][STATE_TRANSPORT_OUT_DRIVE_SIZE];

/**
 * \brief Whether or not the subsystem is initialized.
 */
static BOOL inited = false;

/**
 * \brief A buffer into which packets are received.
 */
static uint8_t buffer[64];

static void on_out(void) {
	uint8_t recipient;
	__data const uint8_t *ptr = buffer;
	uint8_t left = usb_bdpairs[EP_STATE_TRANSPORT].out.BDCNT;

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

	/* Receive the next packet. */
	usb_bdpairs[EP_STATE_TRANSPORT].out.BDCNT = sizeof(buffer);
	usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT = ((usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT & BDSTAT_DTS) ^ BDSTAT_DTS) | BDSTAT_UOWN;
}

void state_transport_out_init(void) {
	if (!inited) {
		/* Set up USB.
		 * The endpoint is halted until the radio channels are set. */
		usb_halted_out_endpoints |= 1 << EP_STATE_TRANSPORT;
		usb_ep_callbacks[EP_STATE_TRANSPORT].out = &on_out;
		usb_bdpairs[EP_STATE_TRANSPORT].out.BDADR = buffer;
		usb_bdpairs[EP_STATE_TRANSPORT].out.BDCNT = sizeof(buffer);
		usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
		UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
		UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 1;

		/* Record state. */
		inited = true;
	}
}

void state_transport_out_deinit(void) {
	if (inited) {
		UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 0;
		usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT = 0;
	}
}

void state_transport_out_halt(void) {
	usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
}

void state_transport_out_unhalt(void) {
	usb_bdpairs[EP_STATE_TRANSPORT].out.BDCNT = sizeof(buffer);
	usb_bdpairs[EP_STATE_TRANSPORT].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
}

