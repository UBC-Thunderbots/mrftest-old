#ifndef DRIBBLER_H
#define DRIBBLER_H

#include "log.h"
#include <stdbool.h>

/**
 * \ingroup DRIBBLER
 *
 * \brief The frequency at which \ref dribbler_tick should be invoked.
 */
#define DRIBBLER_TICK_HZ 25U

/**
 * \ingroup DRIBBLER
 *
 * \brief The possible dribbler operating modes.
 */
typedef enum {
	DRIBBLER_MODE_OFF, ///< The dribbler is not spinning.
	DRIBBLER_MODE_SLOW, ///< The dribbler is spinning slowly.
	DRIBBLER_MODE_FAST, ///< The dribbler is spinning fast.
} dribbler_mode_t;

void dribbler_tick(dribbler_mode_t mode, log_record_t *record);
bool dribbler_hot(void);
unsigned int dribbler_temperature(void);

#endif

