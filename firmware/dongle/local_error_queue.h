#ifndef LOCAL_ERROR_QUEUE_H
#define LOCAL_ERROR_QUEUE_H

/**
 * \file
 *
 * \brief Allows locally-detected errors to be reported back to the host.
 */

#include <stdint.h>

/**
 * \brief Initializes the local error queue subsystem.
 */
void local_error_queue_init(void);

/**
 * \brief Deinitializes the local error queue subsystem.
 */
void local_error_queue_deinit(void);

/**
 * \brief Adds an error to the local error queue.
 *
 * \param[in] error the error to add.
 */
void local_error_queue_add(uint8_t error);

#endif

