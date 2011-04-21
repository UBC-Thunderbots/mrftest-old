#ifndef SHARED_FW_H
#define SHARED_FW_H

/**
 * \file
 *
 * \brief Defines the request codes for the firmware pipe.
 */

#include "params.h"

/**
 * \brief The request codes for the firmware pipe.
 */
typedef enum {
	FIRMWARE_REQUEST_CHIP_ERASE = 0,
	FIRMWARE_REQUEST_FILL_PAGE_BUFFER = 1,
	FIRMWARE_REQUEST_PAGE_PROGRAM = 2,
	FIRMWARE_REQUEST_CRC_BLOCK = 3,
	FIRMWARE_REQUEST_READ_PARAMS = 4,
	FIRMWARE_REQUEST_SET_PARAMS = 5,
	FIRMWARE_REQUEST_COMMIT_PARAMS = 7,
	FIRMWARE_REQUEST_REBOOT = 8,
	FIRMWARE_REQUEST_READ_BUILD_SIGNATURES = 9,
} firmware_request_code_t;

/**
 * \brief The format of a response on the inbound firmware pipe.
 */
typedef struct {
	/**
	 * \brief The length of the micropacket, including its header.
	 */
	uint8_t micropacket_length;

	/**
	 * \brief The pipe number to which the micropacket is addressed.
	 */
	uint8_t pipe;

	/**
	 * \brief The micropacket's sequence number.
	 */
	uint8_t sequence;

	/**
	 * \brief The request code that provoked the response.
	 */
	uint8_t request;

	/**
	 * \brief The response parameters.
	 */
	union {
		/**
		 * \brief The result of a Compute Block CRC command.
		 */
		struct {
			/**
			 * \brief The address of the first byte in the block.
			 */
			uint8_t address[3];

			/**
			 * \brief The number of bytes in the block, or 0 to indicate 64kB.
			 */
			uint16_t length;

			/**
			 * \brief The CRC.
			 */
			uint16_t crc;
		} compute_block_crc_params;

		/**
		 * \brief The result of a Read Operational Parameters Block command.
		 */
		params_t operational_parameters;

		/**
		 * \brief The build signatures.
		 */
		struct {
			/**
			 * \brief The CRC of the PIC firmware.
			 */
			uint16_t firmware_crc;

			/**
			 * \brief The CRC of the SPI flash.
			 */
			uint16_t flash_crc;
		} build_signatures;
	} params;
} firmware_response_t;

#endif

