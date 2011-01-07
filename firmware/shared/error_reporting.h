#ifndef ERROR_REPORTING_H
#define ERROR_REPORTING_H

/**
 * \file
 *
 * \brief Allows errors to be reported back to the host.
 */

#include "../shared/faults.h"
#include <stdint.h>

/**
 * \brief Initializes the error reporting subsystem.
 */
void error_reporting_init(void);

/**
 * \brief Deinitializes the error reporting subsystem.
 */
void error_reporting_deinit(void);

/**
 * \brief Adds an error to the queue.
 *
 * \param[in] error the error to add.
 */
void error_reporting_add(uint8_t error);

#endif

