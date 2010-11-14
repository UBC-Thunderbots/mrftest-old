#include "usb.h"
#include "usb_internal.h"
#include <pic18fregs.h>

/**
 * \file
 *
 * \brief The USB subsystem.
 */

__code const usb_devinfo_t *usb_devinfo;

void usb_init(__code const usb_devinfo_t *devinfo) {
	usb_devinfo = devinfo;
	usb_lowlevel_init();
}

void usb_deinit(void) {
	usb_lowlevel_deinit();
}

