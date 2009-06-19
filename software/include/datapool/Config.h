#ifndef DATAPOOL_CONFIG_H
#define DATAPOOL_CONFIG_H

#include "datapool/Noncopyable.h"
#include <string>
#include <sstream>
#include <glibmm.h>

//
// Handles the configuration file.
//
class Config : private virtual Noncopyable {
public:
	//
	// Loads the configuration file. There can be only one instance.
	//
	Config();

	//
	// Unloads the configuration file.
	//
	~Config();

	//
	// Returns the current instance.
	//
	static Config &instance();

	//
	// Checks whether a particular key exists.
	//
	bool hasKey(const std::string &section, const std::string &key);

	//
	// Gets a string value.
	//
	std::string getString(const std::string &section, const std::string &key);

	//
	// Gets an integer value with a particular radix.
	//
	template<typename T>
	T getInteger(const std::string &section, const std::string &key, unsigned int radix);

private:
	Glib::KeyFile kf;
};

#endif

