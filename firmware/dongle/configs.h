#ifndef CONFIGS_H
#define CONFIGS_H

/**
 * \file
 *
 * \brief Provides access to the various USB configurations supported by the dongle
 */

#include "stdint.h"
#include "usb_ep0.h"

extern const uint8_t CONFIGURATION_DESCRIPTOR1[];
extern const uint8_t CONFIGURATION_DESCRIPTOR2[];
extern const uint8_t CONFIGURATION_DESCRIPTOR3[];
extern const uint8_t CONFIGURATION_DESCRIPTOR6[];
extern const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS1;
extern const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS2;
extern const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS3;
extern const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS6;

#endif

