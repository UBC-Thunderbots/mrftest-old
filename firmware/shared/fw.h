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
	FIRMWARE_REQUEST_CHIP_ERASE,
	FIRMWARE_REQUEST_START_BLOCK,
	FIRMWARE_REQUEST_PAGE_PROGRAM,
	FIRMWARE_REQUEST_READ_PAGE_BITMAP,
	FIRMWARE_REQUEST_CRC_BLOCK,
	FIRMWARE_REQUEST_READ_PARAMS,
	FIRMWARE_REQUEST_SET_PARAMS,
	FIRMWARE_REQUEST_COMMIT_PARAMS,
	FIRMWARE_REQUEST_REBOOT,
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
		 * \brief Bits 16 through 23 of the address of the first byte in a block.
		 */
		uint8_t address_high;

		/**
		 * \brief The result of a Read Page Bitmap command.
		 */
		struct {
			/**
			 * \brief Bits 16 through 23 of the address of the first byte in the current block.
			 */
			uint8_t address_high;

			/**
			 * \brief The page bitmap.
			 */
			uint8_t bitmap[32];
		} read_page_bitmap_params;

		/**
		 * \brief The result of a Compute Block CRC command.
		 */
		struct {
			/**
			 * \brief Bits 16 through 23 of the address of the first byte in the block.
			 */
			uint8_t address_high;

			/**
			 * \brief The CRC.
			 */
			uint16_t crc;
		} compute_block_crc_params;

		/**
		 * \brief The result of a Read Operational Parameters Block command.
		 */
		params_t operational_parameters;
	} params;
} firmware_response_t;

#endif

