#include "descriptors.h"
#include "usb_spec.h"

/**
 * \file
 *
 * \brief Defines the USB descriptors.
 */

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
	/* bNumEndpoints */ 3,
	/* bInterfaceClass */ 0xFF,
	/* bInterfaceSubclass */ 0,
	/* bInterfaceProtocol */ 0,
	/* iInterface */ 0,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x81,
		/* bmAttributes */ 0x03,
		/* wMaxPacketSize */ 4, 0,
		/* bInterval */ 10,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x82,
		/* bmAttributes */ 0x03,
		/* wMaxPacketSize */ 32, 0,
		/* bInterval */ 10,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x83,
		/* bmAttributes */ 0x02,
		/* wMaxPacketSize */ 8, 0,
		/* bInterval */ 10,
	
	/* Interface Descriptor */
	/* bLength */ sizeof(usb_interface_descriptor_t),
	/* bDescriptorType */ USB_DESCRIPTOR_INTERFACE,
	/* bInterfaceNumber */ 1,
	/* bAlternateSetting */ 0,
	/* bNumEndpoints */ 4,
	/* bInterfaceClass */ 0xFF,
	/* bInterfaceSubclass */ 0,
	/* bInterfaceProtocol */ 0,
	/* iInterface */ 0,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x04,
		/* bmAttributes */ 0x03,
		/* wMaxPacketSize */ 64, 0,
		/* bInterval */ 1,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x84,
		/* bmAttributes */ 0x03,
		/* wMaxPacketSize */ 64, 0,
		/* bInterval */ 1,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x05,
		/* bmAttributes */ 0x02,
		/* wMaxPacketSize */ 64, 0,
		/* bInterval */ 0,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x85,
		/* bmAttributes */ 0x02,
		/* wMaxPacketSize */ 64, 0,
		/* bInterval */ 0,
	
	/* Interface Descriptor */
	/* bLength */ sizeof(usb_interface_descriptor_t),
	/* bDescriptorType */ USB_DESCRIPTOR_INTERFACE,
	/* bInterfaceNumber */ 2,
	/* bAlternateSetting */ 0,
	/* bNumEndpoints */ 0,
	/* bInterfaceClass */ 0xFF,
	/* bInterfaceSubclass */ 0,
	/* bInterfaceProtocol */ 0,
	/* iInterface */ 0,
	
	/* Interface Descriptor */
	/* bLength */ sizeof(usb_interface_descriptor_t),
	/* bDescriptorType */ USB_DESCRIPTOR_INTERFACE,
	/* bInterfaceNumber */ 2,
	/* bAlternateSetting */ 1,
	/* bNumEndpoints */ 1,
	/* bInterfaceClass */ 0xFF,
	/* bInterfaceSubclass */ 0,
	/* bInterfaceProtocol */ 0,
	/* iInterface */ 0,

		/* Endpoint Descriptor */
		/* bLength */ sizeof(usb_endpoint_descriptor_t),
		/* bDescriptorType */ USB_DESCRIPTOR_ENDPOINT,
		/* bEndpointAddress */ 0x86,
		/* bmAttributes */ 0x03,
		/* wMaxPacketSize */ 64, 0,
		/* bInterval */ 1,
};

__code const usb_configuration_descriptor_t CONFIGURATION_DESCRIPTOR = {
	/* .length = */ sizeof(usb_configuration_descriptor_t) - sizeof(__code const void *),
	/* .type = */ USB_DESCRIPTOR_CONFIGURATION,
	/* .total_length = */ (uint16_t) sizeof(usb_configuration_descriptor_t) - sizeof(__code const void *) + sizeof(CONFIGURATION_DESCRIPTOR_TAIL),
	/* .num_interfaces = */ 2,
	/* .id = */ 1,
	/* .description_index = */ 0,
	/* .attributes = */ { 0x80 },
	/* .max_power = */ 250,
	/* .following_bytes = */ CONFIGURATION_DESCRIPTOR_TAIL
};



__code const uint8_t STRING_DESCRIPTOR_ZERO[4] = {
	sizeof(STRING_DESCRIPTOR_ZERO),
	USB_DESCRIPTOR_STRING,
	0x09, 0x10, /* 0x1009 = English (Canadian) */
};

static __code const uint16_t STRING_ENUS_MANUFACTURER[] = {
	'U', 'B', 'C', ' ', 'T', 'h', 'u', 'n', 'd', 'e', 'r', 'b', 'o', 't', 's', ' ', 'R', 'o', 'b', 'o', 'c', 'u', 'p', ' ', 'S', 'm', 'a', 'l', 'l', ' ', 'S', 'i', 'z', 'e', ' ', 'T', 'e', 'a', 'm', 0
};

static __code const uint16_t STRING_ENUS_PRODUCT[] = {
	'T', 'h', 'u', 'n', 'd', 'e', 'r', 'b', 'o', 't', 's', ' ', 'X', 'B', 'e', 'e', ' ', 'D', 'o', 'n', 'g', 'l', 'e', 0
};

static __code const usb_string_table_t STRING_TABLE_ENUS = {
	0x1009, /* English (Canadian) */
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

