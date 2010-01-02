#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "xbee/xbee.h"
#include <libxml++/libxml++.h>

//
// The user interface for the tester.
//
class tester_window_impl;
class tester_window {
	public:
		tester_window(xbee &modem, xmlpp::Element *xmlworld);
		~tester_window();

	private:
		tester_window_impl *impl;
};

#endif

