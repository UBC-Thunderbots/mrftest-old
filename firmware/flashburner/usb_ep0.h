#ifndef STM32LIB_USB_EP0_H
#define STM32LIB_USB_EP0_H

/**
 * \file
 *
 * \brief Provides functionality related to USB endpoint zero.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/**
 * \brief A USB SETUP packet as used in a control transfer.
 */
typedef struct {
	/**
	 * \brief The request type, indicating data stage direction, level of specification defining request, and type of recipient.
	 */
	uint8_t request_type;

	/**
	 * \brief The request ID.
	 */
	uint8_t request;

	/**
	 * \brief A numerical value associated with the request.
	 */
	uint16_t value;

	/**
	 * \brief A second numerical value, often used to select a specific element of a collection.
	 */
	uint16_t index;

	/**
	 * \brief The number of bytes the host expects to send or receive in the data stage.
	 */
	uint16_t length;
} usb_ep0_setup_packet_t;

/**
 * \brief The bit field values in the \ref usb_ep0_setup_packet_t.request_type field.
 */
enum {
	/**
	 * \brief Indicates that the data stage will be in the OUT direction or that there will be no data stage.
	 */
	USB_REQ_TYPE_OUT = 0x00,

	/**
	 * \brief Indicates that the data stage will be in the IN direction.
	 */
	USB_REQ_TYPE_IN = 0x80,

	/**
	 * \brief Indicates that the request ID is defined by the core USB standard.
	 */
	USB_REQ_TYPE_STD = 0x00,

	/**
	 * \brief Indicates that the request ID is defined by a standardized device class.
	 */
	USB_REQ_TYPE_CLASS = 0x20,

	/**
	 * \brief Indicates that the request ID is defined by the vendor and is specific to the device.
	 */
	USB_REQ_TYPE_VENDOR = 0x40,

	/**
	 * \brief Indicates that the request is directed at the device as a whole.
	 */
	USB_REQ_TYPE_DEVICE = 0x00,

	/**
	 * \brief Indicates that the request is directed at a single interface selected by \ref usb_ep0_setup_packet_t.index.
	 */
	USB_REQ_TYPE_INTERFACE = 0x01,

	/**
	 * \brief Indicates that the request is directed at a single endpoint selected by \ref usb_ep0_setup_packet_t.index.
	 */
	USB_REQ_TYPE_ENDPOINT = 0x02,
};

/**
 * \brief The request codes defined by the core USB standard.
 */
enum {
	USB_REQ_GET_STATUS = 0,
	USB_REQ_CLEAR_FEATURE = 1,
	USB_REQ_SET_FEATURE = 3,
	USB_REQ_SET_ADDRESS = 5,
	USB_REQ_GET_DESCRIPTOR = 6,
	USB_REQ_SET_DESCRIPTOR = 7,
	USB_REQ_GET_CONFIGURATION = 8,
	USB_REQ_SET_CONFIGURATION = 9,
	USB_REQ_GET_INTERFACE = 10,
	USB_REQ_SET_INTERFACE = 11,
	USB_REQ_SYNCH_FRAME = 12,
};

/**
 * \brief The descriptor types defined by the core USB standard.
 */
enum {
	USB_DTYPE_DEVICE = 1,
	USB_DTYPE_CONFIGURATION = 2,
	USB_DTYPE_STRING = 3,
	USB_DTYPE_INTERFACE = 4,
	USB_DTYPE_ENDPOINT = 5,
	USB_DTYPE_DEVICE_QUALIFIER = 6,
	USB_DTYPE_OTHER_SPEED_CONFIGURATION = 7,
	USB_DTYPE_INTERFACE_POWER = 8,
};

/**
 * \brief The features defined by the core USB standard.
 */
enum {
	USB_FEATURE_ENDPOINT_HALT = 0,
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_TEST_MODE = 2,
};

/**
 * \brief The possible “dispositions” of a control transfer.
 */
typedef enum {
	/**
	 * \brief Indicates that the callback does not care about the transfer.
	 * The transfer will be offered to the next callback.
	 * If every callback returns this disposition for a particular transfer, the transfer will be rejected with STALL.
	 */
	USB_EP0_DISPOSITION_NONE,

	/**
	 * \brief Indicates that the callback will accept this transfer.
	 * No further callbacks will be consulted.
	 * In the case of a no-data transfer (with the exception of a SET ADDRESS request), the side-effects should already have happened.
	 * In the case of an IN-data-stage transfer, a data source must be set up to provide the data, and the status stage will accept.
	 * In the case of an OUT-data-stage transfer, the post-data callback provides a later opportunity to reject the transfer.
	 */
	USB_EP0_DISPOSITION_ACCEPT,

	/**
	 * \brief Indicates that the callback explicitly rejects this transfer.
	 * No further callbacks will be consulted.
	 * In the case of a no-data transfer, the status stage will be rejected with STALL.
	 * In the case of a transfer with a data stage, the data stage will be rejected with STALL.
	 */
	USB_EP0_DISPOSITION_REJECT,
} usb_ep0_disposition_t;

/**
 * \brief A source of data to be sent in an IN-data-stage control transfer.
 */
typedef struct {
	/**
	 * \brief Private data usable by the implementation of the data source.
	 */
	void *opaque;

	/**
	 * \brief Generates some data.
	 *
	 * This callback is mandatory.
	 *
	 * For optimization: note that \p length will always be a multiple of 8 except if one of the following conditions is true:
	 * \li the framework guarantees it will never call the generator again with more buffer space (this happens as the generator nears the end of the amount of data requested by the host), OR
	 * \li the generator previously returned a block of data that was not a multiple of 8 bytes
	 *
	 * This implies that a generator whose natural operation is to generate a block of bytes at a time that is a power of two no more than eight need not arrange to buffer a partially-consumed block.
	 *
	 * For additional convenience, the framework guarantees that even if the host requests a shorter amount of data, the buffer is a multiple of four bytes in length, so the generator may write past the end \em if it only ever generates appropriate numbers of bytes at a time that the pointer cannot become unaligned.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre \p length is nonzero.
	 *
	 * \post \p buffer[0] through \p buffer[\a N - 1] have been populated, if \a N is the return value.
	 *
	 * \param opaque the opaque pointer from the data source structure
	 *
	 * \param buffer the location in which to store generated bytes
	 *
	 * \param length the maximum number of bytes to generate
	 *
	 * \return the number of bytes generated, which may be anything up to \p length but which may only be zero at the end of the data
	 */
	size_t (*generate)(void *opaque, void *buffer, size_t length);
} usb_ep0_source_t;

/**
 * \brief The type of a callback invoked after an OUT data stage of a transfer is complete.
 *
 * This callback is invoked in callback context but not under global NAK.
 *
 * \return \c true to accept the request, or \c false to reject the request by returning STALL handshake in the status stage
 */
typedef bool (*usb_ep0_postdata_cb_t)(void);

/**
 * \brief The type of a callback invoked after the status stage of a control transfer is complete.
 *
 * This callback is invoked in callback context but not under global NAK.
 */
typedef void (*usb_ep0_poststatus_cb_t)(void);

/**
 * \brief A set of callbacks to handle control transfers.
 */
typedef struct {
	/**
	 * \brief Handles a control transfer with no data stage.
	 *
	 * This callback is optional; if not provided, it is treated as though always returning \ref USB_EP0_DISPOSITION_NONE.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 *
	 * \param setup_packet the SETUP packet
	 *
	 * \param[out] poststatus a poststatus callback (if \ref USB_EP0_DISPOSITION_ACCEPT)
	 *
	 * \return the disposition of the request
	 */
	usb_ep0_disposition_t (*on_zero_request)(const usb_ep0_setup_packet_t *setup_packet, usb_ep0_poststatus_cb_t *poststatus);

	/**
	 * \brief Handles a control transfer with an IN data stage.
	 *
	 * This callback is optional; if not provided, it is treated as though always returning \ref USB_EP0_DISPOSITION_NONE.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 *
	 * \param setup_packet the SETUP packet
	 *
	 * \param[out] source the source of data to return (if \ref USB_EP0_DISPOSITION_ACCEPT)
	 *
	 * \param[out] poststatus a poststatus callback (if \ref USB_EP0_DISPOSITION_ACCEPT)
	 *
	 * \return the disposition of the request
	 */
	usb_ep0_disposition_t (*on_in_request)(const usb_ep0_setup_packet_t *setup_packet, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *poststatus);

	/**
	 * \brief Handles a control transfer with an OUT data stage.
	 *
	 * This callback is optional; if not provided, it is treated as though always returning \ref USB_EP0_DISPOSITION_NONE.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 *
	 * \param setup_packet the SETUP packet *
	 *
	 * \param[out] dest the location into which to store the data sent by the host (if \ref USB_EP0_DISPOSITION_ACCEPT)
	 *
	 * \param[out] postdata an postdata callback (if \ref USB_EP0_DISPOSITION_ACCEPT), assumed to return \c true if not provided
	 *
	 * \param[out] poststatus a poststatus callback (if \ref USB_EP0_DISPOSITION_ACCEPT)
	 *
	 * \return the disposition of the request
	 */
	usb_ep0_disposition_t (*on_out_request)(const usb_ep0_setup_packet_t *setup_packet, void **dest, usb_ep0_postdata_cb_t *postdata, usb_ep0_poststatus_cb_t *poststatus);
} usb_ep0_cbs_t;

/**
 * \brief The maximum number of callback structures that can be registered at the same time.
 */
#define USB_EP0_CBS_STACK_SIZE 8

/**
 * \brief Initializes the endpoint zero module.
 *
 * This should generally be done as a result of an enumeration complete interrupt from the low level module.
 * After calling this function, the application should push some callback structures onto the stack to handle requests.
 *
 * \param max_packet the maximum packet size for endpoint zero
 */
void usb_ep0_init(size_t max_packet);

/**
 * \brief Shuts down the endpoint zero module.
 */
void usb_ep0_deinit(void);

/**
 * \brief Pushes a callbacks structure on the callback handling stack.
 *
 * When handling a request, callbacks are invoked from newest (most recently pushed) to oldest.
 *
 * \param cbs the callbacks structure to push
 */
void usb_ep0_cbs_push(const usb_ep0_cbs_t *cbs);

/**
 * \brief Removes a callbacks structure from the callback handling stack.
 *
 * It is not necessary to remove callbacks structures in the opposite order they are pushed.
 * It is also safe to call this function with a structure that is not yet registered.
 *
 * \param cbs the callbacks structure to remove
 */
void usb_ep0_cbs_remove(const usb_ep0_cbs_t *cbs);

#endif

