#ifndef TESTER_CONTROLS_H
#define TESTER_CONTROLS_H

#include "xbee/packettypes.h"

class tester_controls {
	public:
		virtual void encode(xbeepacket::RUN_DATA &data) = 0;
};

#endif

