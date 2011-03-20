#ifndef MESSAGE_IN_H
#define MESSAGE_IN_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of inbound message packet reception.
 */

#include <stdint.h>

/**
 * \brief Initializes the subsystem.
 */
void message_in_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void message_in_deinit(void);

/**
 * \brief Sends an inbound message message to the host.
 *
 * This function returns once the message has been delivered.
 *
 * \param[in] message the message to send, which must already have the appropriate header.
 *
 * \param[in] len the length of the message.
 */
void message_in_send(__data const uint8_t *message, uint8_t len);

#endif

