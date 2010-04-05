#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <libxml++/libxml++.h>

//
// Manages the "config.xml" file.
//
namespace config {
	//
	// Gets the configuration data. The data is loaded from disk if this has
	// not yet been done.
	//
	xmlpp::Document *get();

	//
	// Enqueues a save to disk. The data will not be written immediately;
	// rather, a timer will be started and the data will be written in a few
	// seconds. This avoids many repeated writes within a very short time if
	// a lot of data is being changed by different modules. However, write
	// starvation will also not occur: if writes are occurring constantly,
	// the data will still be written to disk every few seconds.
	//
	void dirty();

	//
	// Forces the current data to be written to disk immediately. Normally
	// you should call "save()" instead; the exception is if the application
	// is shutting down.
	//
	void force_save();
}

#endif

