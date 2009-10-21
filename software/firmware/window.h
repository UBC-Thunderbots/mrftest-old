#ifndef FIRMWARE_WINDOW_H
#define FIRMWARE_WINDOW_H

#include "xbee/packetproto.h"
#include <libxml++/libxml++.h>

//
// The user interface for the firmware manager.
//
class firmware_window_impl;
class firmware_window {
	public:
		firmware_window(xbee_packet_stream &xbee, xmlpp::Element *xmlworld);
		~firmware_window();

	private:
		firmware_window_impl *impl;
};

#endif

