#ifndef FLASH_H
#define FLASH_H

/**
 * \file
 *
 * \brief Provides the ability to read and write the parameters block.
 */

#include <stddef.h>
#include <stdint.h>



/**
 * \brief A location at which the parameters block can be read.
 */
extern const volatile uint8_t * const flash_params_block;

/**
 * \brief Writes new data to the parameters block.
 *
 * \param[in] data the data to write
 *
 * \param[in] len the number of bytes to write
 */
void flash_write_params(const void *params, size_t len);

#endif

