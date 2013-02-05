#ifndef USB_EP0_SOURCES_H
#define USB_EP0_SOURCES_H

/**
 * \file
 *
 * \brief Provides a collection of useful data sources for endpoint 0 data transmission.
 */

#include "stddef.h"
#include "stdint.h"
#include "usb_ep0.h"



/**
 * \name Memory block source.
 *
 * This data source returns the contents of a block of bytes addressable through a pointer.
 *
 * @{
 */

/**
 * \brief A memory block source.
 *
 * The contents of this structure are private.
 * The application should not access them.
 */
typedef struct {
	const uint8_t *ptr;
	size_t len;
	usb_ep0_source_t src;
} usb_ep0_memory_source_t;

/**
 * \brief Sets up a memory block source.
 *
 * \param source the source to configure, which must have storage allocated
 *
 * \param data the block of data to return
 *
 * \param length the length of \p data, in bytes
 *
 * \return the configured source pointer, ready to use
 */
usb_ep0_source_t *usb_ep0_memory_source_init(usb_ep0_memory_source_t *source, const void *data, size_t length);

/**
 * @}
 */



/**
 * \name Generator that produces a USB-compliant string descriptor from a UTF-8 literal string.
 *
 * This data source reads a NUL-terminated UTF-8 string from memory and returns, as its output, a USB-compliant string descriptor.
 *
 * @{
 */

/**
 * \brief A UTF-8 to UTF-16 transcoding source.
 *
 * The contents of this structure are private.
 * The application should not access them.
 */
typedef struct {
	const unsigned char *ptr;
	uint8_t descriptor_length, flags;
	uint16_t pending_trail_surrogate;
	usb_ep0_source_t src;
} usb_ep0_string_descriptor_source_t;

/**
 * \brief Sets up a string descriptor generating source.
 *
 * \param source the source to configure, which must have storage allocated
 *
 * \param string the string to transcode
 *
 * \return the configured source pointer, ready to use
 */
usb_ep0_source_t *usb_ep0_string_descriptor_source_init(usb_ep0_string_descriptor_source_t *source, const char *string);

#endif

