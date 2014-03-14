#ifndef HOST_CONTROLLED_H
#define HOST_CONTROLLED_H

#include <stdint.h>
#include "usb_configs.h"

extern const uint8_t TARGET_CONFIGURATION_DESCRIPTOR[], ONBOARD_CONFIGURATION_DESCRIPTOR[];
extern const usb_configs_config_t TARGET_CONFIGURATION, ONBOARD_CONFIGURATION;

#endif

