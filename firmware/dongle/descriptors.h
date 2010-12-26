#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "usb.h"

__code extern const usb_device_descriptor_t DEVICE_DESCRIPTOR;
__code extern const usb_configuration_descriptor_t CONFIGURATION_DESCRIPTOR;
__code extern const uint8_t CONFIGURATION_DESCRIPTOR_TAIL[];
__code extern const uint8_t STRING_DESCRIPTOR_ZERO[4];
__code extern const usb_string_metatable_t STRING_METATABLE;

#endif

