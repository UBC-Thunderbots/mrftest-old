#ifndef PARAMS_H
#define PARAMS_H

/**
 * \file
 *
 * \brief Provides the operational parameters block.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief The possible contents of the SPI flash.
 */
typedef enum {
	/**
	 * \brief The flash contains no useful data.
	 */
	FLASH_CONTENTS_NONE,

	/**
	 * \brief The flash contains an FPGA bitstream.
	 */
	FLASH_CONTENTS_FPGA,

	/**
	 * \brief The flash contains a PIC firmware upgrade.
	 */
	FLASH_CONTENTS_PIC,
} flash_contents_t;

/**
 * \brief The type of the operational parameters block.
 */
typedef struct {
	/**
	 * \brief The contents of the SPI flash chip.
	 */
	flash_contents_t flash_contents;

	/**
	 * \brief The XBee radio channels.
	 */
	uint8_t xbee_channels[2];

	/**
	 * \brief The robot number.
	 */
	uint8_t robot_number;
} params_t;

/**
 * \brief The RAM shadow of the operational parameters block.
 */
extern params_t params;

/**
 * \brief Loads the parameters from flash into RAM.
 *
 * \return \c true on success, or \c false on failure (e.g. if the parameters are corrupt).
 */
BOOL params_load(void);

#endif

