#ifndef USB_SPEC_H
#define USB_SPEC_H

/**
 * \file
 *
 * \brief The structures and constants defined by the USB Specification.
 */

#include <stdint.h>

/**
 * \brief The possible types of descriptors.
 */
typedef enum {
	/**
	 * \brief A device descriptor.
	 */
	USB_DESCRIPTOR_DEVICE = 1,

	/**
	 * \brief A configuration descriptor.
	 */
	USB_DESCRIPTOR_CONFIGURATION = 2,

	/**
	 * \brief A string descriptor.
	 */
	USB_DESCRIPTOR_STRING = 3,

	/**
	 * \brief An interface descriptor.
	 */
	USB_DESCRIPTOR_INTERFACE = 4,

	/**
	 * \brief An endpoint descriptor.
	 */
	USB_DESCRIPTOR_ENDPOINT = 5,

	/**
	 * \brief An interface association descriptor.
	 */
	USB_DESCRIPTOR_INTERFACE_ASSOCIATION = 11,
} usb_descriptor_type_t;

/**
 * \brief A USB device descriptor.
 */
typedef struct {
	/**
	 * \brief The length of the descriptor in bytes.
	 */
	uint8_t length;

	/**
	 * \brief The descriptor type code (must be USB_DESCRIPTOR_DEVICE).
	 */
	uint8_t type;

	/**
	 * \brief The version of the USB specification implemented by this device.
	 */
	uint16_t usb_version;

	/**
	 * \brief The class implemented by the device as a whole.
	 */
	uint8_t device_class;

	/**
	 * \brief The subclass implemented by the device as a whole.
	 */
	uint8_t device_subclass;

	/**
	 * \brief The protocol implemented by the device as a whole.
	 */
	uint8_t device_protocol;

	/**
	 * \brief The maximum packet size on endpoint zero (must be 8 for this framework).
	 */
	uint8_t ep0_max_packet;

	/**
	 * \brief The vendor ID.
	 */
	uint16_t vendor_id;

	/**
	 * \brief The product ID.
	 */
	uint16_t product_id;

	/**
	 * \brief The release number of the device.
	 */
	uint16_t device_version;

	/**
	 * \brief The index of the string containing the manufacturer.
	 */
	uint8_t manufacturer_index;

	/**
	 * \brief The index of the string containing the product name.
	 */
	uint8_t product_index;

	/**
	 * \brief The index of the string containing the serial number.
	 */
	uint8_t serial_index;

	/**
	 * \brief The number of configurations in the device.
	 */
	uint8_t num_configurations;
} usb_device_descriptor_t;

/**
 * \brief The layout of the attributes field of a configuration descriptor, broken down into bits.
 */
typedef struct {
	/**
	 * \brief Reserved (set to zero).
	 */
	unsigned reserved0 : 5;

	/**
	 * \brief \c 1 if the device supports remote wakeup in the configuration, or \c 0 if not.
	 */
	unsigned remote_wakeup : 1;

	/**
	 * \brief \c 1 if the device uses power from an external source in the configuration, or \c 0 if it uses power only from the bus.
	 */
	unsigned self_powered : 1;

	/**
	 * \brief Reserved (set to one).
	 */
	unsigned reserved1 : 1;
} usb_configuration_attributes_bits_t;

/**
 * \brief The attributes field of a configuration descriptor.
 */
typedef union {
	/**
	 * \brief The value as a byte.
	 */
	uint8_t byte;

	/**
	 * \brief The value as a bitfield.
	 */
	usb_configuration_attributes_bits_t bits;
} usb_configuration_attributes_t;

/**
 * \brief A USB configuration descriptor.
 */
typedef struct {
	/**
	 * \brief The length of the configuration descriptor only in bytes.
	 */
	uint8_t length;

	/**
	 * \brief The descriptor type code (must be USB_DESCRIPTOR_CONFIGURATION for a configuration descriptor).
	 */
	uint8_t type;

	/**
	 * \brief The length of the configuration descriptor and all following descriptors in bytes.
	 */
	uint16_t total_length;

	/**
	 * \brief The number of interfaces in the configuration.
	 */
	uint8_t num_interfaces;

	/**
	 * \brief The value identifying the configuration.
	 */
	uint8_t id;

	/**
	 * \brief The index of the string describing the configuration.
	 */
	uint8_t description_index;

	/**
	 * \brief The attributes of the configuration.
	 */
	usb_configuration_attributes_t attributes;

	/**
	 * \brief The maximum amount of power consumed in the configuration, in units of 2mA.
	 */
	uint8_t max_power;

	/**
	 * \brief The bytes making up the other descriptors that are sent following the configuration descriptor.
	 */
	__code const void *following_bytes;
} usb_configuration_descriptor_t;

/**
 * \brief A USB interface descriptor.
 */
typedef struct {
	/**
	 * \brief The length of the interface descriptor in bytes.
	 */
	uint8_t length;

	/**
	 * \brief The descriptor type code (must be USB_DESCRIPTOR_INTERFACE for an interface descriptor).
	 */
	uint8_t type;

	/**
	 * \brief The interface number.
	 */
	uint8_t id;

	/**
	 * \brief The alternate setting number.
	 */
	uint8_t alternate_setting;

	/**
	 * \brief The number of endpoints, excluding endpoint zero.
	 */
	uint8_t num_endpoints;

	/**
	 * \brief The interface class.
	 */
	uint8_t interface_class;

	/**
	 * \brief The interface subclass.
	 */
	uint8_t interface_subclass;

	/**
	 * \brief The interface protocol.
	 */
	uint8_t interface_protocol;

	/**
	 * \brief The index of the string describing the interface.
	 */
	uint8_t interface_index;
} usb_interface_descriptor_t;

/**
 * \brief A USB endpoint descriptor.
 */
typedef struct {
	/**
	 * \brief The length of the endpoint descriptor only bytes.
	 */
	uint8_t length;

	/**
	 * \brief The descriptor type code (must be USB_DESCRIPTOR_ENDPOINT for an endpoint descriptor).
	 */
	uint8_t type;

	/**
	 * \brief The endpoint address.
	 */
	uint8_t endpoint;

	/**
	 * \brief The endpoint attributes.
	 */
	uint8_t attributes;

	/**
	 * \brief The maximum size of packet handled by the endpoint.
	 */
	uint16_t max_packet;

	/**
	 * \brief The polling interval for the endpoint.
	 */
	uint8_t interval;
} usb_endpoint_descriptor_t;

/**
 * \brief The possible types of recipients for a SETUP packet.
 */
typedef enum {
	/**
	 * \brief Indicates that the packet is addressed to the device as a whole.
	 */
	USB_SETUP_PACKET_RECIPIENT_DEVICE = 0,

	/**
	 * \brief Indicates that the packet is addressed to a particular interface.
	 */
	USB_SETUP_PACKET_RECIPIENT_INTERFACE = 1,

	/**
	 * \brief Indicates that the packet is addressed to a particular endpoint.
	 */
	USB_SETUP_PACKET_RECIPIENT_ENDPOINT = 2,

	/**
	 * \brief Indicates that the packet is addressed to something other than the device or an interface or endpoint.
	 */
	USB_SETUP_PACKET_RECIPIENT_OTHER = 3,
} usb_setup_packet_request_type_recipient_t;

/**
 * \brief The possible types of requests for a SETUP packet.
 */
typedef enum {
	/**
	 * \brief Indicates that the packet is for a USB-standard request.
	 */
	USB_SETUP_PACKET_REQUEST_STANDARD = 0,

	/**
	 * \brief Indicates that the packet is for a class-specific request.
	 */
	USB_SETUP_PACKET_REQUEST_CLASS = 1,

	/**
	 * \brief Indicates that the packet is for a vendor-specific request.
	 */
	USB_SETUP_PACKET_REQUEST_VENDOR = 2,
} usb_setup_packet_request_type_request_t;

/**
 * \brief The standard control transfer requests.
 */
typedef enum {
	USB_SETUP_PACKET_STDREQ_GET_STATUS = 0,
	USB_SETUP_PACKET_STDREQ_CLEAR_FEATURE = 1,
	USB_SETUP_PACKET_STDREQ_SET_FEATURE = 3,
	USB_SETUP_PACKET_STDREQ_SET_ADDRESS = 5,
	USB_SETUP_PACKET_STDREQ_GET_DESCRIPTOR = 6,
	USB_SETUP_PACKET_STDREQ_GET_CONFIGURATION = 8,
	USB_SETUP_PACKET_STDREQ_SET_CONFIGURATION = 9,
	USB_SETUP_PACKET_STDREQ_GET_INTERFACE = 10,
} usb_setup_packet_standard_request_t;

/**
 * \brief The standard feature selectors.
 */
typedef enum {
	USB_SETUP_PACKET_STDFEAT_ENDPOINT_HALT = 0,
	USB_SETUP_PACKET_STDFEAT_DEVICE_REMOTE_WAKEUP = 1,
	USB_SETUP_PACKET_STDFEAT_DEVICE_TEST_MODE = 2,
} usb_setup_packet_standard_feature_t;

/**
 * \brief The layout of a SETUP packet's request_type, broken down into bits.
 */
typedef struct {
	/**
	 * \brief The type of recipient.
	 */
	usb_setup_packet_request_type_recipient_t recipient : 5;

	/**
	 * \brief The type of request.
	 */
	usb_setup_packet_request_type_request_t type : 2;

	/**
	 * \brief The direction of the data stage (0 for OUT or 1 for IN).
	 */
	unsigned direction : 1;
} usb_setup_packet_request_type_bits_t;

/**
 * \brief The layout of a SETUP packet's request_type.
 */
typedef union {
	/**
	 * \brief The value as a byte.
	 */
	uint8_t byte;

	/**
	 * \brief The value as a bitfield.
	 */
	usb_setup_packet_request_type_bits_t bits;
} usb_setup_packet_request_type_t;

/**
 * \brief The layout of a SETUP packet.
 */
typedef struct {
	/**
	 * \brief The request type.
	 */
	usb_setup_packet_request_type_t request_type;

	/**
	 * \brief The specific request.
	 */
	uint8_t request;

	/**
	 * \brief A request-specific 16-bit value.
	 */
	uint16_t value;

	/**
	 * \brief A request-specific 16-bit value often used to identify an endpoint or interface.
	 */
	uint16_t index;

	/**
	 * \brief The number of bytes the host wishes to transfer in the data stage.
	 */
	uint16_t length;
} usb_setup_packet_t;

#endif

