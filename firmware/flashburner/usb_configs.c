#include "usb_configs.h"
#include <assert.h>
#include <stddef.h>
#include <unused.h>
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

static const usb_configs_config_t * const *configs = 0;
static uint8_t current_config = 0;
static const usb_configs_config_t *current_config_struct = 0;



static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_SET_CONFIGURATION) {
		if (!pkt->index) {
			if (pkt->value) {
				// SET CONFIGURATION(nonzero)
				for (const usb_configs_config_t * const *i = configs; *i; ++i) {
					if (pkt->value == (*i)->configuration) {
						if (!(*i)->can_enter || (*i)->can_enter()) {
							if (current_config_struct && current_config_struct->on_exit) {
								current_config_struct->on_exit();
							}
							current_config_struct = *i;
							current_config = current_config_struct->configuration;
							if (current_config_struct->on_enter) {
								current_config_struct->on_enter();
							}
							return USB_EP0_DISPOSITION_ACCEPT;
						}
					}
				}
			} else {
				// SET CONFIGURATION(0)
				if (current_config_struct && current_config_struct->on_exit) {
					current_config_struct->on_exit();
				}
				current_config = 0;
				current_config_struct = 0;
				return USB_EP0_DISPOSITION_ACCEPT;
			}
		}
		return USB_EP0_DISPOSITION_REJECT;
	}

	return USB_EP0_DISPOSITION_NONE;
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_GET_CONFIGURATION) {
		if (!pkt->value && !pkt->index && pkt->length == 1) {
			*source = usb_ep0_memory_source_init(&mem_src, &current_config, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	}

	return USB_EP0_DISPOSITION_NONE;
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

void usb_configs_init(const usb_configs_config_t * const *cfgs) {
	// Sanity check.
	assert(cfgs);

	// Set up variables.
	configs = cfgs;
	current_config = 0;
	current_config_struct = 0;

	// Register callbacks.
	usb_ep0_cbs_push(&EP0_CBS);
}

void usb_configs_deinit(void) {
	// If currently in a configuration, leave it.
	if (current_config_struct && current_config_struct->on_exit) {
		current_config_struct->on_exit();
	}

	// Clear variables.
	configs = 0;
	current_config = 0;
	current_config_struct = 0;

	// Unregister callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);
}

uint8_t usb_configs_get_current(void) {
	return current_config;
}

