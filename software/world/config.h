#ifndef WORLD_CONFIG_H
#define WORLD_CONFIG_H

#include <libxml++/document.h>

//
// Manages the "config.xml" file.
//
class config {
	public:
		//
		// Gets the configuration data. The data is loaded from disk if this has
		// not yet been done.
		//
		static xmlpp::Document *get();

		//
		// Writes the current configuration data to disk.
		//
		static void save();

	private:
		config();
};

#endif

