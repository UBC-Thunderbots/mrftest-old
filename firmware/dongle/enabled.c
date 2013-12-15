#include "enabled.h"
#include "buzzer.h"
#include "config.h"
#include "constants.h"
#include "normal.h"
#include "radio_off.h"
#include "promiscuous.h"
#include <core_progmem.h>
#include <deferred.h>
#include <init.h>
#include <registers/scb.h>
#include <unused.h>
#include <usb_altsettings.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_ll.h>

#define USB_DTYPE_DFU 0x21

enum {
	DFU_DETACH = 0,
	DFU_DNLOAD = 1,
	DFU_UPLOAD = 2,
	DFU_GETSTATUS = 3,
	DFU_CLRSTATUS = 4,
	DFU_GETSTATE = 5,
	DFU_ABORT = 6,
};

const uint8_t ENABLED_CONFIGURATION_DESCRIPTOR[] = {
	/* CONFIGURATION DESCRIPTOR */
	9, // bLength
	USB_DTYPE_CONFIGURATION, // bDescriptorType
	103, // wTotalLength LSB
	0, // wTotalLength MSB
	INTERFACE_COUNT, // bNumInterfaces
	1, // bConfigurationValue
	0, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	/* INTERFACE DESCRIPTOR, INTERFACE 0, ALTERNATE SETTING 0, Radio Off */
	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	INTERFACE_RADIO, // bInterfaceNumber
	RADIO_ALTSETTING_OFF, // bAlternateSetting
	0, // bNumEndpoints
	0xFF, // bInterfaceClass
	SUBCLASS_RADIO, // bInterfaceSubClass
	RADIO_PROTOCOL_OFF, // bInterfaceProtocol
	STRING_INDEX_RADIO_OFF, // iInterface

	/* INTERFACE DESCRIPTOR, INTERFACE 0, ALTERNATE SETTING 1, Normal Mode */
	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	INTERFACE_RADIO, // bInterfaceNumber
	RADIO_ALTSETTING_NORMAL, // bAlternateSetting
	6, // bNumEndpoints
	0xFF, // bInterfaceClass
	SUBCLASS_RADIO, // bInterfaceSubClass
	RADIO_PROTOCOL_NORMAL, // bInterfaceProtocol
	STRING_INDEX_NORMAL, // iInterface

		/* ENDPOINT DESCRIPTOR, ENDPOINT 1 OUT, Drive Data */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x01, // bEndpointAddress
		0x03, // bmAttributes
		64, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		5, // bInterval

		/* ENDPOINT DESCRIPTOR, ENDPOINT 2 OUT, Queue Reliable Message */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x02, // bEndpointAddress
		0x03, // bmAttributes
		64, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		5, // bInterval

		/* ENDPOINT DESCRIPTOR, ENDPOINT 3 OUT, Queue Unreliable Message */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x03, // bEndpointAddress
		0x03, // bmAttributes
		64, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		5, // bInterval

		/* ENDPOINT DESCRIPTOR, ENDPOINT 1 IN, Message Delivery Reports */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x81, // bEndpointAddress
		0x02, // bmAttributes
		8, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		0, // bInterval

		/* ENDPOINT DESCRIPTOR, ENDPOINT 2 IN, Received Messages */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x82, // bEndpointAddress
		0x03, // bmAttributes
		64, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		5, // bInterval

		/* ENDPOINT DESCRIPTOR, ENDPOINT 3 IN, Hardware Run Switch */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x83, // bEndpointAddress
		0x03, // bmAttributes
		1, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		10, // bInterval

	/* INTERFACE DESCRIPTOR, INTERFACE 0, ALTERNATE SETTING 2, Promiscuous Mode */
	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	INTERFACE_RADIO, // bInterfaceNumber
	RADIO_ALTSETTING_PROMISCUOUS, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	SUBCLASS_RADIO, // bInterfaceSubClass
	RADIO_PROTOCOL_PROMISCUOUS, // bInterfaceProtocol
	STRING_INDEX_PROMISCUOUS, // iInterface

		/* ENDPOINT DESCRIPTOR, ENDPOINT 1 IN, Received Frames */
		7, // bLength
		USB_DTYPE_ENDPOINT, // bDescriptorType
		0x81, // bEndpointAddress
		0x02, // bmAttributes
		64, // wMaxPacketSize LSB
		0, // wMaxPacketSize MSB
		0, // bInterval

	/* INTERFACE DESCRIPTOR, INTERFACE 1, ALTERNATE SETTING 0, DFU */
	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	INTERFACE_DFU, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFE, // bInterfaceClass
	0x01, // bInterfaceSubClass
	1, // bInterfaceProtocol
	0, // iInterface

		/* DFU FUNCTIONAL DESCRIPTOR */
		9, // bLength
		USB_DTYPE_DFU, // bDescriptorType
		0b00001011, // bmAttributes
		255, 0, // wDetachTimeout
		0, 8, // wTransferSize
		0x1A, 0x01, // bcdDFUVersion
};

static void dfu_detach_reboot(void *UNUSED(cookie)) {
	init_bootload();
}

static void dfu_detach_gnak2(void) {
	static deferred_fn_t def = DEFERRED_FN_INIT;

	// Detach from the USB.
	usb_ll_detach();

	// Set a deferred function to reboot into the bootloader once we unwind out of current interrupts.
	deferred_fn_register(&def, &dfu_detach_reboot, 0);
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
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_BEEP) {
		// Start the buzzer.
		buzzer_start(pkt->value);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_DFU && pkt->request == DFU_DETACH) {
		// We will reboot into the bootloader after the transfer’s status stage finishes.
		*poststatus = &dfu_detach_poststatus;

		return USB_EP0_DISPOSITION_ACCEPT;
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

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_GET_CHANNEL) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the channel number.
		*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_GET_SYMBOL_RATE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the symbol rate.
		*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_GET_PAN_ID) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the PAN ID.
		*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_RADIO && pkt->request == CONTROL_REQUEST_GET_MAC_ADDRESS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the MAC address.
		*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_READ_CORE) {
		// This request reads a 1-kibibyte block.
		// The value field identifies which block to read.
		if (pkt->value >= 256) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the data.
		*source = usb_ep0_memory_source_init(&mem_src, &core_progmem_dump[pkt->value * 1024U / 4U], 1024);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_DFU && pkt->request == DFU_GETSTATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Send all six bytes of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, DFU_STATUS, sizeof(DFU_STATUS));
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->index == INTERFACE_DFU && pkt->request == DFU_GETSTATE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Send only bState out of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, &DFU_STATUS[4], 1);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// This request must be addressed to an existent interface.
		if (pkt->index >= INTERFACE_COUNT) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Interface status is always all zeroes.
		*source = usb_ep0_memory_source_init(&mem_src, ZEROES, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

static const usb_altsettings_altsetting_t * const INT0_ALTSETTINGS[] = {
	&RADIO_OFF_ALTSETTING,
	&NORMAL_ALTSETTING,
	&PROMISCUOUS_ALTSETTING,
	0,
};

static void on_enter(void) {
	usb_altsettings_init(0, INT0_ALTSETTINGS);
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	usb_ep0_cbs_remove(&EP0_CBS);
	usb_altsettings_deinit(0);
	buzzer_stop();
}

const usb_configs_config_t ENABLED_CONFIGURATION = {
	.configuration = 1,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

