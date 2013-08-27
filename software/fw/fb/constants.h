#ifndef FW_FB_CONSTANTS_H
#define FW_FB_CONSTANTS_H

/**
 * \brief The vendor ID used by the burner.
 */
#define FLASH_BURNER_VID 0x0483

/**
 * \brief The product ID used by the burner.
 */
#define FLASH_BURNER_PID 0x497D

/**
 * \brief The interface subclass number used by the burner in target mode using polled writes.
 */
#define FLASH_BURNER_POLLED_TARGET_SUBCLASS 0

/**
 * \brief The interface subclass number used by the burner in target mode using interrupt-completed writes.
 */
#define FLASH_BURNER_INTERRUPT_TARGET_SUBCLASS 0

/**
 * \brief The interface subclass number used by the burner in on-board mode using polled writes.
 */
#define FLASH_BURNER_POLLED_ONBOARD_SUBCLASS 0

/**
 * \brief The interface subclass number used by the burner in on-board mode using interrupt-completed writes.
 */
#define FLASH_BURNER_INTERRUPT_ONBOARD_SUBCLASS 0

/**
 * \brief The interface subclass number used by the burner in UART mode.
 */
#define FLASH_BURNER_UART_SUBCLASS 0

/**
 * \brief The interface protocol number used by the burner in burn mode (on-board or target).
 *
 * This number acts as a version number and will change if incompatible protocol changes are made, thus ensuring software and firmware match capabilities.
 */
#define FLASH_BURNER_BURN_PROTOCOL 1

/**
 * \brief The interface protocol number used by the burner in UART mode.
 *
 * This number acts as a version number and will change if incompatible protocol changes are made, thus ensuring software and firmware match capabilities.
 */
#define FLASH_BURNER_UART_PROTOCOL 0xFF

enum {
	STRING_INDEX_ZERO = 0,
	STRING_INDEX_MANUFACTURER,
	STRING_INDEX_PRODUCT,
	STRING_INDEX_CONFIG1,
	STRING_INDEX_CONFIG2,
	STRING_INDEX_CONFIG3,
	STRING_INDEX_CONFIG4,
	STRING_INDEX_SERIAL,
};

enum {
	CONTROL_REQUEST_READ_IO,
	CONTROL_REQUEST_WRITE_IO,
	CONTROL_REQUEST_JEDEC_ID,
	CONTROL_REQUEST_READ_STATUS,
	CONTROL_REQUEST_READ,
	CONTROL_REQUEST_WRITE,
	CONTROL_REQUEST_ERASE,
	CONTROL_REQUEST_GET_ERRORS,
};

enum {
	SELECT_TARGET_STATUS_OK = 0,
	SELECT_TARGET_STATUS_UNRECOGNIZED_JEDEC_ID = 1,
};

enum {
	PIN_MOSI = 1,
	PIN_MISO = 2,
	PIN_CLOCK = 4,
	PIN_CS = 8,
	PIN_POWER = 16,
	PIN_PROGRAM_B = 32,
};

#endif

