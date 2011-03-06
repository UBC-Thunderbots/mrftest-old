#ifndef PARAMS_H
#define PARAMS_H

/**
 * \file
 *
 * \brief Provides the operational parameters block.
 */

#include "../shared/params.h"
#include <stdbool.h>

/**
 * \brief The RAM shadow of the operational parameters block.
 */
extern params_t params;

/**
 * \brief The CRC16 of the firmware.
 */
extern uint16_t firmware_crc;

/**
 * \brief The CRC16 of the SPI flash.
 */
extern uint16_t flash_crc;

/**
 * \brief Loads the parameters from flash into RAM.
 *
 * \return \c true on success, or \c false on failure (e.g. if the parameters are corrupt).
 */
BOOL params_load(void);

/**
 * \brief Saves the current in-memory parameters to flash.
 */
void params_commit(void);

#endif

