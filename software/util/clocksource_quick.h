#ifndef UTIL_CLOCKSOURCE_QUICK_H
#define UTIL_CLOCKSOURCE_QUICK_H

#include "util/clocksource.h"

//
// A clock source that fires as quickly as possible.
//
class clocksource_quick : public clocksource, public sigc::trackable {
	public:
		//
		// Constructs a new clocksource.
		//
		clocksource_quick();

	private:
		bool on_idle();
};

#endif

