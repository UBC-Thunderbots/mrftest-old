#include "descriptors.h"
#include "usb_spec.h"

/**
 * \file
 *
 * \brief Defines the USB descriptors.
 */

__code const uint8_t ESTOP_REPORT_DESCRIPTOR[27] = {
	0x05, 0xFF,       /* USAGE_PAGE(Vendor-defined 0xFF) */
	0x09, 0x00,       /* USAGE(Vendor usage 0) */
	0xA1, 0x01,       /* COLLECTION(Application) */
	0x15, 0x00,       /*   LOGICAL_MINIMUM(0) */
	0x25, 0x01,       /*   LOGICAL_MAXIMUM(1) */
	0x75, 0x01,       /*   REPORT_SIZE(1) */
	0x95, 0x01,       /*   REPORT_COUNT(1) */
	0x05, 0x09,       /*   USAGE_PAGE(Button) */
	0x09, 0x01,       /*   USAGE(Button 1) */
	0x81, 0x02,       /*   INPUT(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Non-volatile, Bit Field) */
	0x75, 0x01,       /*   REPORT_SIZE(1) */
	0x95, 0x07,       /*   REPORT_COUNT(7) */
	0x81, 0x03,       /*   INPUT(Constant, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Non-volatile, Bit Field) */
	0xC0,             /* END_COLLECTION */
};

__code const usb_device_descriptor_t DEVICE_DESCRIPTOR = {
	/* .length = */ sizeof(usb_device_descriptor_t),
	/* .type = */ USB_DESCRIPTOR_DEVICE,
	/* .usb_version = */ 0x0200,
	/* .device_class = */ 0,
	/* .device_subclass = */ 0,
	/* .device_protocol = */ 0,
	/* .ep0_max_packet = */ 8,
	/* .vendor_id = */ 0x04D8,
	/* .product_id = */ 0x7839,
	/* .device_version = */ 0x0100,
	/* .manufacturer_index = */ 1,
	/* .product_index = */ 2,
	/* .serial_index = */ 0,
	/* .num_configurations = */ 1,
};

__code const uint8_t CONFIGURATION_DESCRIPTOR_TAIL[] = {
	/* Interface Descriptor */
	/* bLength */ sizeof(usb_interface_descriptor_t),
	/* bDescriptorType */ USB_DESCRIPTOR_INTERFACE,
	/* bInterfaceNumber */ 0,
	/* bAlternateSetting */ 0,
	/* bNumEndpoints */ 1,
	/* bInterfaceClass */ 3,
	/* bInterfaceSubclass */ 0,
	/* bInterfaceProtocol */ 0,
	/* iInterface */ 0,

	/* HID Descriptor */
	/* bLength */ 9,
	/* bDescriptorType */ 0x21,
	/* bcdHID */ 0x11, 0x01,
	/* bCountryCode */ 0,
	/* bNumDescriptors */ 1,
	/* bDescriptorType */ 0x22,
	/* wDescriptorLength */ sizeof(ESTOP_REPORT_DESCRIPTOR) & 0xFF, ((uint16_t) sizeof(ESTOP_REPORT_DESCRIPTOR)) >> 8,

	/* Endpoint Descriptor */
	/* bLength */ sizeof(usb_endpoint_descriptor_t),
	/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
	/* bEndpointAddress */ 0x81,
	/* bmAttributes */ 0x03,
	/* wMaxPacketSize */ 1, 0,
	/* bInterval */ 10,
};

__code const usb_configuration_descriptor_t CONFIGURATION_DESCRIPTOR = {
	/* .length = */ sizeof(usb_configuration_descriptor_t) - sizeof(__code const void *),
	/* .type = */ USB_DESCRIPTOR_CONFIGURATION,
	/* .total_length = */ (uint16_t) sizeof(usb_configuration_descriptor_t) - sizeof(__code const void *) + sizeof(CONFIGURATION_DESCRIPTOR_TAIL),
	/* .num_interfaces = */ 1,
	/* .id = */ 1,
	/* .description_index = */ 0,
	/* .attributes = */ { 0x80 },
	/* .max_power = */ 250,
	/* .following_bytes = */ CONFIGURATION_DESCRIPTOR_TAIL
};



__code const uint8_t STRING_DESCRIPTOR_ZERO[4] = {
	sizeof(STRING_DESCRIPTOR_ZERO),
	USB_DESCRIPTOR_STRING,
	0x09, 0x04, /* 0x0409 = English (United States) */
};

static __code const uint16_t STRING_ENUS_MANUFACTURER[] = {
	'C', 'h', 'r', 'i', 's', 't', 'o', 'p', 'h', 'e', 'r', ' ', 'H', 'e', 'a', 'd', 0
};

static __code const uint16_t STRING_ENUS_PRODUCT[] = {
	'T', 'h', 'u', 'n', 'd', 'e', 'r', 'b', 'o', 't', 's', ' ', 'X', 'B', 'e', 'e', ' ', 'D', 'o', 'n', 'g', 'l', 'e', 0
};

static __code const usb_string_table_t STRING_TABLE_ENUS = {
	0x0409, /* English (United States) */
	{
		STRING_ENUS_MANUFACTURER,
		STRING_ENUS_PRODUCT,
	},
};

__code const usb_string_metatable_t STRING_METATABLE = {
	1, /* # of tables */
	2, /* # of strings in each table */
	{ &STRING_TABLE_ENUS, },
};

