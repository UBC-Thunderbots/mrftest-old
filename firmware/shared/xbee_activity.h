#ifndef SHARED_XBEE_ACTIVITY_H
#define SHARED_XBEE_ACTIVITY_H

/**
 * \file
 *
 * \brief The API that must be implemented by an individual target to achieve XBee activity monitoring.
 */

#include <stdint.h>

/**
 * \brief Initializes activity monitoring.
 */
void xbee_activity_init(void);

/**
 * \brief Deinitializes activity monitoring.
 */
void xbee_activity_deinit(void);

/**
 * \brief Registers activity for an XBee.
 *
 * \param[in] xbee the index of the XBee on which activity occurred.
 */
void xbee_activity_mark(uint8_t xbee);

#endif

