#include "idle.h"
#include "constants.h"
#include <core_progmem.h>
#include <deferred.h>
#include <init.h>
#include <registers/scb.h>
#include <stdint.h>
#include <unused.h>
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

#define USB_DTYPE_DFU 0x21

const uint8_t IDLE_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	USB_DTYPE_CONFIGURATION, // bDescriptorType
	27, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	1, // bConfigurationValue
	STRING_INDEX_CONFIG1, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

	9, // bLength
	USB_DTYPE_INTERFACE, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFE, // bInterfaceClass
	0x01, // bInterfaceSubClass
	1, // bInterfaceProtocol
	0, // iInterface

	9, // bLength
	USB_DTYPE_DFU, // bDescriptorType
	0b00001011, // bmAttributes
	255, 0, // wDetachTimeout
	0, 8, // wTransferSize
	0x1A, 0x01, // bcdDFUVersion
};

extern volatile uint64_t bootload_flag;

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
	if (pkt->request_type == (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == DFU_DETACH) {
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

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_READ_CORE) {
		// This request reads a 1-kibibyte block.
		// The value field identifies which block to read.
		if (pkt->value >= 256) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the data.
		*source = usb_ep0_memory_source_init(&mem_src, &core_progmem_dump[pkt->value * 1024U / 4U], 1024);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == DFU_GETSTATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Send all six bytes of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, DFU_STATUS, sizeof(DFU_STATUS));

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == DFU_GETSTATE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Send only bState out of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, &DFU_STATUS[4], 1);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_INTERFACE) {
		// This request must have value set to zero.
		if (pkt->value) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Return the alternate setting number, which is always zero.
		*source = usb_ep0_memory_source_init(&mem_src, ZEROES, 1);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && !pkt->index && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value set to zero.
		if (pkt->value) {
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

static void on_enter(void) {
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	usb_ep0_cbs_remove(&EP0_CBS);
}

const usb_configs_config_t IDLE_CONFIGURATION = {
	.configuration = 1,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

