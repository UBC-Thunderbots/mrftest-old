#include "config.h"
#include "configs.h"
#include "constants.h"
#include "registers.h"
#include "stdint.h"
#include "unused.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

const uint8_t CONFIGURATION_DESCRIPTOR1[] = {
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

static void dfu_detach_poststatus(void) {
	usb_detach();
	bootload_flag = UINT64_C(0xDEADBEEFCAFEBABE);
}

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept, usb_ep0_poststatus_callback_t *poststatus) {
	if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_CHANNEL && 0x0B <= value && value <= 0x1A && !index) {
		config.channel = value;
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_SYMBOL_RATE && (value == 0x00 || value == 0x01) && !index) {
		config.symbol_rate = value;
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_PAN_ID && value != 0xFFFF && !index) {
		config.pan_id = value;
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_CLASS | USB_STD_REQ_TYPE_INTERFACE) && request == 0 && index == 0) {
		// DFU_DETACH
		*poststatus = &dfu_detach_poststatus;
		*accept = true;
		return true;
	} else {
		return false;
	}
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t UNUSED(length), usb_ep0_source_t **source, usb_ep0_poststatus_callback_t *UNUSED(poststatus)) {
	static const uint8_t DFU_STATUS[] = {
		0x00, // bStatus (OK)
		10, 0, 0, // bwPollTimeout
		0, // bState (appIDLE)
		0, // iString
	};
	static usb_ep0_memory_source_t mem_src;

	if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_CHANNEL && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_SYMBOL_RATE && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_PAN_ID && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_MAC_ADDRESS && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_CLASS | USB_STD_REQ_TYPE_INTERFACE) && request == 3 && index == 0) {
		// DFU_GETSTATUS
		// Send all six bytes of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, DFU_STATUS, sizeof(DFU_STATUS));
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_CLASS | USB_STD_REQ_TYPE_INTERFACE) && request == 5 && index == 0) {
		// DFU_GETSTATE
		// Send only bState out of the status block.
		*source = usb_ep0_memory_source_init(&mem_src, &DFU_STATUS[4], 1);
		return true;
	} else {
		return false;
	}
}

static union {
	uint64_t mac_address;
} control_out_buffer;

static bool out_request_set_mac_address_cb(void) {
	config.mac_address = control_out_buffer.mac_address;
	return true;
}

static bool on_out_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void **dest, bool (**cb)(void), usb_ep0_poststatus_callback_t *UNUSED(poststatus)) {
	if (request_type == 0x40 && request == 0x07 && !value && !index && length == 8) {
		*dest = &control_out_buffer.mac_address;
		*cb = &out_request_set_mac_address_cb;
		return true;
	} else {
		return false;
	}
}

const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS1 = {
	.configuration = 1,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 0,
	.can_enter = 0,
	.on_enter = 0,
	.on_exit = 0,
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = &on_out_request,
};

