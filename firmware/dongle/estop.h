#ifndef ESTOP_H
#define ESTOP_H

#include "signal.h"

/**
 * \brief Initializes the emergency stop connector and begins sampling the switch.
 *
 * This function does not return until a sample has been taken and the state initilialized.
 */
void estop_init(void);

/**
 * \brief Stops sampling the emergency stop switch.
 */
void estop_deinit(void);

#endif

