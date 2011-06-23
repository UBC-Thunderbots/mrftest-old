#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <libxml++/libxml++.h>

namespace Config {
	void load();
	void save();
	xmlpp::Element *params();
	xmlpp::Element *joysticks();
	xmlpp::Element *robots();
}

#endif

