#include "radio_sleep.h"
#include "config.h"
#include "constants.h"
#include "registers.h"
#include "stdint.h"
#include "unused.h"
#include "usb_configs.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"
#include "usb_ll.h"

enum {
	DFU_DETACH = 0,
	DFU_DNLOAD = 1,
	DFU_UPLOAD = 2,
	DFU_GETSTATUS = 3,
	DFU_CLRSTATUS = 4,
	DFU_GETSTATE = 5,
	DFU_ABORT = 6,
};

const uint8_t RADIO_SLEEP_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	2, // bDescriptorType
	27, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	1, // bConfigurationValue
	STRING_INDEX_CONFIG1, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFE, // bInterfaceClass
	0x01, // bInterfaceSubClass
	1, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	0x21, // bDescriptorType
	0b00001011, // bmAttributes
	255, 0, // wDetachTimeout
	0, 8, // wTransferSize
	0x1A, 0x01, // bcdDFUVersion
};

extern volatile uint64_t bootload_flag;

static void dfu_detach_gnak2(void) {
	// Detach from the USB.
	usb_ll_detach();

	// Mark that we should go to the bootloader on next reboot.
	bootload_flag = UINT64_C(0xFE228106195AD2B0);

	// We will reboot when the main loop notices that the USB is detached.
}

static void dfu_detach_gnak1(void) {
	// Re-request global NAK as per the convention for detaching, to ensure any other global NAKs have been processed.
	static usb_ll_gnak_req_t gnak_req = USB_LL_GNAK_REQ_INIT;
	usb_ll_set_gnak(&gnak_req, &dfu_detach_gnak2);
}

static void dfu_detach_poststatus(void) {
	// Get into global NAK mode.
	static usb_ll_gnak_req_t gnak_req = USB_LL_GNAK_REQ_INIT;
	usb_ll_set_gnak(&gnak_req, &dfu_detach_gnak1);
}

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *poststatus) {
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_CHANNEL) {
		if (0x0B <= pkt->value && pkt->value <= 0x1A && !pkt->index) {
			config.channel = pkt->value;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_SYMBOL_RATE) {
		if ((pkt->value == 0x00 || pkt->value == 0x01) && !pkt->index) {
			config.symbol_rate = pkt->value;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_PAN_ID) {
		if (pkt->value != 0xFFFF && !pkt->index) {
			config.pan_id = pkt->value;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->request == DFU_DETACH) {
		if (!pkt->index) {
			*poststatus = &dfu_detach_poststatus;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static const uint8_t DFU_STATUS[] = {
		0x00, // bStatus (OK)
		10, 0, 0, // bwPollTimeout
		0, // bState (appIDLE)
		0, // iString
	};
	static const uint8_t ZEROES[2] = { 0, 0 };
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_CHANNEL) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_SYMBOL_RATE) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_PAN_ID) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_MAC_ADDRESS) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->request == DFU_GETSTATUS) {
		if (!pkt->value && !pkt->index) {
			// Send all six bytes of the status block.
			*source = usb_ep0_memory_source_init(&mem_src, DFU_STATUS, sizeof(DFU_STATUS));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->request == DFU_GETSTATE) {
		if (!pkt->value && !pkt->index) {
			// Send only bState out of the status block.
			*source = usb_ep0_memory_source_init(&mem_src, &DFU_STATUS[4], 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE) {
		if (!pkt->value && !pkt->index && pkt->length == 1) {
			*source = usb_ep0_memory_source_init(&mem_src, ZEROES, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS) {
		if (!pkt->value && !pkt->index && pkt->length == 2) {
			*source = usb_ep0_memory_source_init(&mem_src, ZEROES, 2);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
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
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_MAC_ADDRESS) {
		if (!pkt->value && !pkt->index && pkt->length == 8) {
			*dest = &control_out_buffer.mac_address;
			*postdata = &out_request_set_mac_address_postdata;
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = &on_out_request,
};

static void on_enter(void) {
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	usb_ep0_cbs_remove(&EP0_CBS);
}

const usb_configs_config_t RADIO_SLEEP_CONFIGURATION = {
	.configuration = 1,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

