#ifndef DRIBBLER_H
#define DRIBBLER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Whether or not the dribbler is requested to be operating.
 */
extern bool dribbler_enabled;

/**
 * \brief The speed of the dribbler.
 */
extern uint8_t dribbler_speed;

/**
 * \brief The estimated thermal energy of the dribbler winding, in joules.
 */
extern float dribbler_winding_energy;

/**
 * \brief The estimated thermal energy of the dribbler housing, in joules.
 */
extern float dribbler_housing_energy;

/**
 * \brief Updates the dribbler.
 *
 * \param battery the battery voltage
 */
void dribbler_tick(float battery);

#endif

