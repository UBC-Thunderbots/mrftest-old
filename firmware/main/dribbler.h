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

void dribbler_tick(uint8_t power, log_record_t *record);
bool dribbler_hot(void);
unsigned int dribbler_temperature(void);

#endif

