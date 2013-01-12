#include "config.h"
#include "configs.h"
#include "constants.h"
#include "registers.h"
#include "stdint.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

const uint8_t CONFIGURATION_DESCRIPTOR1[] = {
	9, // bLength
	2, // bDescriptorType
	18, // wTotalLength LSB
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
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface
};

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept) {
	if (request_type == 0x40 && request == 0x01 && 0x0B <= value && value <= 0x1A && !index) {
		config.channel = value;
		*accept = true;
		return true;
	} else if (request_type == 0x40 && request == 0x03 && (value == 0x00 || value == 0x01) && !index) {
		config.symbol_rate = value;
		*accept = true;
		return true;
	} else if (request_type == 0x40 && request == 0x05 && value != 0xFFFF && !index) {
		config.pan_id = value;
		*accept = true;
		return true;
	} else {
		return false;
	}
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length __attribute__((unused)), usb_ep0_source_t **source) {
	static usb_ep0_memory_source_t mem_src;

	if (request_type == 0xC0 && request == 0x00 && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
		return true;
	} else if (request_type == 0xC0 && request == 0x02 && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
		return true;
	} else if (request_type == 0xC0 && request == 0x04 && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
		return true;
	} else if (request_type == 0xC0 && request == 0x06 && !value && !index) {
		*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
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

static bool on_out_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void **dest, bool (**cb)(void)) {
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

