#include "radio_off.h"
#include "config.h"
#include "constants.h"
#include <stdint.h>
#include <unused.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_SET_CHANNEL) {
		// This request must have value set to a valid channel.
		if (pkt->value < 0x0B || pkt->value > 0x1A) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Save the channel.
		config.channel = pkt->value;

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_SET_SYMBOL_RATE) {
		// This request must have value set to a valid symbol rate.
		if (pkt->value > 2) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Save the symbol rate.
		config.symbol_rate = pkt->value;

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_SET_PAN_ID) {
		// This request must have value set to a valid PAN ID.
		if (pkt->value == 0xFFFF) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Save the PAN ID.
		config.pan_id = pkt->value;

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static union {
	uint64_t mac_address;
} control_out_buffer;

static bool out_request_set_mac_address_postdata(void) {
	config.mac_address = control_out_buffer.mac_address;
	return true;
}

static usb_ep0_disposition_t on_out_request(const usb_ep0_setup_packet_t *pkt, void **dest, usb_ep0_postdata_cb_t *postdata, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_SET_MAC_ADDRESS) {
		// This request must have value set to zero and a length of eight.
		if (pkt->value || pkt->length != 8) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Set up a write buffer.
		*dest = &control_out_buffer.mac_address;
		*postdata = &out_request_set_mac_address_postdata;
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_out_request = &on_out_request,
};

static void on_enter(void) {
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	usb_ep0_cbs_remove(&EP0_CBS);
}

const usb_altsettings_altsetting_t RADIO_OFF_ALTSETTING = {
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

