#include "usb_altsettings.h"
#include <assert.h>
#include <stddef.h>
#include <unused.h>
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

static const usb_altsettings_altsetting_t * const *altsettings[USB_ALTSETTINGS_MAX_INTERFACES] = { 0 };
static uint8_t num_altsettings[USB_ALTSETTINGS_MAX_INTERFACES] = { 0 };
static uint8_t current_altsetting[USB_ALTSETTINGS_MAX_INTERFACES] = { 0 };
static unsigned int num_enabled_interfaces = 0;



static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_SET_INTERFACE) {
		// Check that it’s an interface we’re registered to handle.
		if (pkt->index < USB_ALTSETTINGS_MAX_INTERFACES && altsettings[pkt->index]) {
			// Check that the requested alternate setting exists.
			if (pkt->value < num_altsettings[pkt->index]) {
				// Check if it’s OK to enter the target alternate setting.
				if (!altsettings[pkt->index][pkt->value]->can_enter || altsettings[pkt->index][pkt->value]->can_enter()) {
					// Exit the current alternate setting.
					if (altsettings[pkt->index][current_altsetting[pkt->index]]->on_exit) {
						altsettings[pkt->index][current_altsetting[pkt->index]]->on_exit();
					}
					// Enter the new alternate setting.
					current_altsetting[pkt->index] = pkt->value;
					if (altsettings[pkt->index][pkt->value]->on_enter) {
						altsettings[pkt->index][pkt->value]->on_enter();
					}
					return USB_EP0_DISPOSITION_ACCEPT;
				} else {
					return USB_EP0_DISPOSITION_REJECT;
				}
			} else {
				return USB_EP0_DISPOSITION_REJECT;
			}
		}
	}

	return USB_EP0_DISPOSITION_NONE;
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE) {
		// Check that it’s an interface we’re registered to handle.
		if (pkt->index < USB_ALTSETTINGS_MAX_INTERFACES && altsettings[pkt->index]) {
			// Check for required parameters.
			if (!pkt->value && pkt->length == 1) {
				// Return the current alternate setting.
				*source = usb_ep0_memory_source_init(&mem_src, &current_altsetting[pkt->index], 1);
				return USB_EP0_DISPOSITION_ACCEPT;
			} else {
				return USB_EP0_DISPOSITION_REJECT;
			}
		}
	}

	return USB_EP0_DISPOSITION_NONE;
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

void usb_altsettings_init(unsigned int interface, const usb_altsettings_altsetting_t * const *as) {
	// Sanity check.
	assert(interface < USB_ALTSETTINGS_MAX_INTERFACES);
	assert(as);
	assert(as[0]);

	// If this interface already had alternate settings, shut them down.
	usb_altsettings_deinit(interface);

	// Set up variables.
	altsettings[interface] = as;
	current_altsetting[interface] = 0;

	// Count the number of available alternate settings.
	for (num_altsettings[interface] = 0; as[num_altsettings[interface]]; ++num_altsettings[interface]);

	// Register callbacks.
	if (!num_enabled_interfaces) {
		usb_ep0_cbs_push(&EP0_CBS);
	}

	// Record this interface as enabled.
	++num_enabled_interfaces;

	// Enter the zeroth alternate setting.
	if (altsettings[interface][0]->on_enter) {
		altsettings[interface][0]->on_enter();
	}
}

void usb_altsettings_deinit(unsigned int interface) {
	// Sanity check.
	assert(interface < USB_ALTSETTINGS_MAX_INTERFACES);

	// If this interface was not configured for alternate settings at all, do nothing.
	if (!altsettings[interface]) {
		return;
	}

	// Leave the current alternate setting.
	if (altsettings[interface][current_altsetting[interface]]->on_exit) {
		altsettings[interface][current_altsetting[interface]]->on_exit();
	}

	// Clear variables.
	altsettings[interface] = 0;
	current_altsetting[interface] = 0;

	// Record this interface as disabled.
	--num_enabled_interfaces;

	// Unregister callbacks.
	if (!num_enabled_interfaces) {
		usb_ep0_cbs_remove(&EP0_CBS);
	}
}

uint8_t usb_altsettings_get_current(unsigned int interface) {
	// Sanity check.
	assert(interface < USB_ALTSETTINGS_MAX_INTERFACES);

	// Return the current value.
	return current_altsetting[interface];
}

