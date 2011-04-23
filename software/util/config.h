#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <libxml++/libxml++.h>

namespace Config {
	xmlpp::Document *get();
	void load();
	void save();
}

#endif

