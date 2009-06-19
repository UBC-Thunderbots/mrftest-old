#include "datapool/Config.h"
#include "Log/Log.h"

#include <sstream>
#include <cassert>
#include <cstdlib>
#include <stdint.h>

namespace {
	Config *inst;
}

Config::Config() {
	// There should only be one instance.
	assert(!inst);

	// Build the name of the config file.
	std::string homeDir = Glib::getenv("HOME");
	if (homeDir == "") {
		Log::log(Log::LEVEL_ERROR, "XBee") << "Environment variable $HOME is not set!\n";
		std::exit(1);
	}
	std::string configFileName = homeDir + "/.thunderbots/thunderbots.conf";

	// Load the file.
	kf.load_from_file(configFileName, Glib::KEY_FILE_NONE);

	// Save the instance.
	inst = this;
}

Config::~Config() {
	// Record destruction.
	inst = 0;
}

Config &Config::instance() {
	return *inst;
}

bool Config::hasKey(const std::string &section, const std::string &key) {
	return kf.has_key(section, key);
}

std::string Config::getString(const std::string &section, const std::string &key) {
	if (!kf.has_group(section)) {
		Log::log(Log::LEVEL_ERROR, "Config") << "The configuration file does not contain the required section [" << section << "]\n";
		std::exit(1);
	}

	if (!kf.has_key(section, key)) {
		Log::log(Log::LEVEL_ERROR, "Config") << "The configuration file does not contain the required key '" << key << "' in the section [" << section << "]\n";
		std::exit(1);
	}

	return kf.get_value(section, key);
}

template<typename T>
T Config::getInteger(const std::string &section, const std::string &key, unsigned int radix) {
	const std::string &str = getString(section, key);
	std::istringstream iss(str);
	assert(radix == 8 || radix == 10 || radix == 16);
	switch (radix) {
		case 8:  iss.setf(std::ios_base::oct, std::ios_base::basefield); break;
		case 10: iss.setf(std::ios_base::dec, std::ios_base::basefield); break;
		case 16: iss.setf(std::ios_base::hex, std::ios_base::basefield); break;
	}
	T result;
	iss >> result;
	return result;
}

// Instantiate the template for all the necessary data types.
template unsigned int Config::getInteger<unsigned int>(const std::string &, const std::string &, unsigned int);
template uint64_t Config::getInteger<uint64_t>(const std::string &, const std::string &, unsigned int);
