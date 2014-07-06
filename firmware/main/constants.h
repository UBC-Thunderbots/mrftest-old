#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * \brief The vendor ID.
 */
#define VENDOR_ID 0x0483U

/**
 * \brief The product ID.
 */
#define PRODUCT_ID 0x497EU

/**
 * \brief The interface numbers.
 */
enum {
	INTERFACE_CDC_ACM,
	INTERFACE_COUNT,
};

/**
 * \brief The string indices understood by a GET DESCRIPTOR(String) request.
 */
enum {
	STRING_INDEX_ZERO = 0U,
	STRING_INDEX_MANUFACTURER,
	STRING_INDEX_PRODUCT,
	STRING_INDEX_SERIAL,
};

/**
 * \brief The vendor-specific control requests understood by the robot.
 */
enum {
	CONTROL_REQUEST_READ_CORE = 0x0DU,
};

#endif

