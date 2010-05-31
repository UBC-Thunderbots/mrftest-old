#ifndef FIRMWARE_WINDOW_H
#define FIRMWARE_WINDOW_H

#include "util/scoped_ptr.h"
#include <libxml++/libxml++.h>

//
// The user interface for the firmware manager.
//
class firmware_window_impl;
class firmware_window {
	public:
		firmware_window(xbee &modem, xmlpp::Element *xmlworld);
		~firmware_window();

	private:
		scoped_ptr<firmware_window_impl> impl;
};

#endif

