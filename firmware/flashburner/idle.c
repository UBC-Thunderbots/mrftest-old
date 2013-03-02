#include "idle.h"
#include "constants.h"
#include <deferred.h>
#include <registers.h>
#include <stdint.h>
#include <unused.h>
#include <usb_configs.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_ll.h>

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
	// Mark that we should go to the bootloader on next reboot.
	bootload_flag = UINT64_C(0xFE228106195AD2B0);

	// Disable all interrupts.
	asm volatile("cpsid i");
	asm volatile("isb");

	// Request the reboot.
	SCS_AIRCR = (SCS_AIRCR & ~VECTKEY(0xFFFF)) | VECTKEY(0x05FA) | SYSRESETREQ;
	asm volatile("dsb");

	// Wait forever until the reboot happens.
	for (;;) {
		asm volatile("wfi");
	}
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
	if (pkt->request_type == (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->request == DFU_DETACH) {
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

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE) && pkt->request == DFU_GETSTATUS) {
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

