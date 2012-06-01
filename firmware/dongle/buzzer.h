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
 * \brief Turns the buzzer on or off
 *
 * \param[in] ctl \c true to turn the buzzer on, or \c false to turn it off
 */
void buzzer_set(bool ctl);

#endif

