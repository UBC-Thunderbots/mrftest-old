#ifndef CHANNELS_H
#define CHANNELS_H

#include <stdint.h>

/**
 * \brief The channels the XBees should communicate on.
 */
__code __at(0x7800) extern uint8_t channels[2];

/**
 * \brief Saves a new choice of channels.
 *
 * \param[in] channel0 the channel for XBee 0.
 *
 * \param[in] channel1 the channel for XBee 1.
 */
void channels_change(uint8_t channel0, uint8_t channel1);

#endif

