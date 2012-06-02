#ifndef BUZZER_H
#define BUZZER_H

#include "stdbool.h"

/**
 * \file
 *
 * \brief Handles the alert buzzer
 */

/**
 * \brief Initializes the buzzer system
 */
void buzzer_init(void);

/**
 * \brief Ensures the buzzer runs for at least a specified length of time
 *
 * The buzzer will stop sounding at the last end time across all calls to this function.
 *
 * \param[in] millis the number of milliseconds to buzz for, starting from the current time
 */
void buzzer_start(unsigned long millis);

#endif

