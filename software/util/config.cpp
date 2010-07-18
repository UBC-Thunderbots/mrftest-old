#include "util/algorithm.h"
#include "util/config.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <glibmm.h>
#include <libgen.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <ext/functional>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	//
	// A unary functor that accepts a RobotInfo and returns its address.
	//
	class RobotAddress : public std::unary_function<const Config::RobotInfo &, uint64_t> {
		public:
			uint64_t operator()(const Config::RobotInfo &bot) const {
				return bot.address;
			}
	};

	/**
	 * A binary predicate that compares two robots by their 64-bit addresses.
	 */
	class CompareByAddress : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				return x.address < y.address;
			}
	};

	/**
	 * A binary predicate that compares two robots by their lid patterns.
	 */
	class CompareByLid : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				if (x.yellow != y.yellow) {
					return x.yellow < y.yellow;
				} else {
					return x.pattern_index < y.pattern_index;
				}
			}
	};

	/**
	 * A binary predicate that compares two robots by their names.
	 */
	class CompareByName : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				return x.name < y.name;
			}
	};

	/**
	 * @return the directory in which the running executable is stored.
	 */
	std::string get_bin_directory() {
		std::vector<char> buffer(64);
		ssize_t retval;
		while ((retval = readlink("/proc/self/exe", &buffer[0], buffer.size())) == static_cast<ssize_t>(buffer.size())) {
			buffer.resize(buffer.size() * 2);
		}
		if (retval < 0) {
			throw std::runtime_error("Cannot get path to binary!");
		}
		if (retval < static_cast<ssize_t>(buffer.size())) {
			buffer[retval] = '\0';
		} else {
			buffer.push_back('\0');
		}
		return dirname(&buffer[0]);
	}

	/**
	 * @return the filename of the config file.
	 */
	std::string get_filename() {
		return get_bin_directory() + "/config.dat";
	}
}

Config::Config() : channel_(0x0E) {
	// Load file.
	std::ifstream ifs;
	try {
		ifs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		const std::string &filename = get_filename();
		ifs.open(filename.c_str(), std::ios::in | std::ios::binary);
		char signature[8];
		ifs.read(signature, sizeof(signature));
		if (std::equal(signature, signature + 8, "TBOTC001")) {
			load_v1(ifs);
		} else if (std::equal(signature, signature + 8, "TBOTC002")) {
			load_v2(ifs);
		} else if (std::equal(signature, signature + 8, "TBOTC003")) {
			load_v3(ifs);
		} else {
			// Unknown version number. Give up.
		}
	} catch (const std::ifstream::failure &exp) {
		// Swallow.
	}
}

void Config::save() const {
	// Save file.
	std::ofstream ofs;
	ofs.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	const std::string &filename = get_filename();
	ofs.open(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	ofs.write("TBOTC003", 8);
	robots_.save(ofs);
	{
		uint8_t ch = channel_;
		ofs.write(reinterpret_cast<const char *>(&ch), 1);
	}

	uint32_t nparams;

	nparams = bool_params.size();
	ofs.write(reinterpret_cast<const char *>(&nparams), sizeof(nparams));
	for (typeof(bool_params.begin()) i = bool_params.begin(); i != bool_params.end(); ++i) {
		const std::string &name(i->first.raw());
		uint32_t namelen = name.size();
		ofs.write(reinterpret_cast<const char *>(&namelen), sizeof(namelen));
		ofs.write(name.data(), name.size());
		char value = i->second ? 1 : 0;
		ofs.write(&value, sizeof(value));
	}

	nparams = int_params.size();
	ofs.write(reinterpret_cast<const char *>(&nparams), sizeof(nparams));
	for (typeof(int_params.begin()) i = int_params.begin(); i != int_params.end(); ++i) {
		const std::string &name(i->first.raw());
		uint32_t namelen = name.size();
		ofs.write(reinterpret_cast<const char *>(&namelen), sizeof(namelen));
		ofs.write(name.data(), name.size());
		int value = i->second;
		ofs.write(reinterpret_cast<const char *>(&value), sizeof(value));
	}

	nparams = double_params.size();
	ofs.write(reinterpret_cast<const char *>(&nparams), sizeof(nparams));
	for (typeof(double_params.begin()) i = double_params.begin(); i != double_params.end(); ++i) {
		const std::string &name(i->first.raw());
		uint32_t namelen = name.size();
		ofs.write(reinterpret_cast<const char *>(&namelen), sizeof(namelen));
		ofs.write(name.data(), name.size());
		double value = i->second;
		ofs.write(reinterpret_cast<const char *>(&value), sizeof(value));
	}
}

void Config::channel(unsigned int chan) {
	assert(0x0B <= chan && chan <= 0x1A);
	channel_ = chan;
}

void Config::load_v1(std::istream &ifs) {
	robots_.load_v1(ifs);
	channel_ = 0x0E;
}

void Config::load_v2(std::istream &ifs) {
	robots_.load_v2(ifs);
	{
		uint8_t ch;
		ifs.read(reinterpret_cast<char *>(&ch), 1);
		channel(ch);
	}
}

void Config::load_v3(std::istream &ifs) {
	robots_.load_v2(ifs);
	{
		uint8_t ch;
		ifs.read(reinterpret_cast<char *>(&ch), 1);
		channel(ch);
	}

	uint32_t nparams;

	ifs.read(reinterpret_cast<char *>(&nparams), 4);
	while (nparams--) {
		uint32_t namelen;
		ifs.read(reinterpret_cast<char *>(&namelen), 4);
		char buffer[namelen];
		ifs.read(buffer, sizeof(buffer));
		Glib::ustring name(buffer, sizeof(buffer));
		char value;
		ifs.read(&value, sizeof(value));
		bool_params[name] = !!value;
	}

	ifs.read(reinterpret_cast<char *>(&nparams), 4);
	while (nparams--) {
		uint32_t namelen;
		ifs.read(reinterpret_cast<char *>(&namelen), 4);
		char buffer[namelen];
		ifs.read(buffer, sizeof(buffer));
		Glib::ustring name(buffer, sizeof(buffer));
		int value;
		ifs.read(reinterpret_cast<char *>(&value), sizeof(value));
		int_params[name] = value;
	}

	ifs.read(reinterpret_cast<char *>(&nparams), 4);
	while (nparams--) {
		uint32_t namelen;
		ifs.read(reinterpret_cast<char *>(&namelen), 4);
		char buffer[namelen];
		ifs.read(buffer, sizeof(buffer));
		Glib::ustring name(buffer, sizeof(buffer));
		double value;
		ifs.read(reinterpret_cast<char *>(&value), sizeof(value));
		double_params[name] = value;
	}
}

const Config::RobotInfo &Config::RobotSet::find(uint64_t address) const {
	for (typeof(robots.begin()) i(robots.begin()), iend(robots.end()); i != iend; ++i) {
		if (i->address == address) {
			return *i;
		}
	}
	throw std::runtime_error("Cannot find robot by address!");
}

const Config::RobotInfo &Config::RobotSet::find(const Glib::ustring &name) const {
	for (typeof(robots.begin()) i(robots.begin()), iend(robots.end()); i != iend; ++i) {
		if (i->name == name) {
			return *i;
		}
	}
	throw std::runtime_error("Cannot find robot by name!");
}

bool Config::RobotSet::contains_address(uint64_t address) const {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->address == address) {
			return true;
		}
	}
	return false;
}

bool Config::RobotSet::contains_pattern(bool yellow, unsigned int pattern_index) const {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->yellow == yellow && i->pattern_index == pattern_index) {
			return true;
		}
	}
	return false;
}

bool Config::RobotSet::contains_name(const Glib::ustring &name) const {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->name == name) {
			return true;
		}
	}
	return false;
}

void Config::RobotSet::add(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name) {
	assert(!contains_address(address));
	assert(!contains_pattern(yellow, pattern_index));
	assert(!name.empty());
	assert(!contains_name(name));
	unsigned int index = robots.size();
	robots.push_back(RobotInfo(address, yellow, pattern_index, name));
	signal_robot_added.emit(index);
}

void Config::RobotSet::remove(uint64_t address) {
	std::vector<RobotInfo>::iterator i = std::find_if(robots.begin(), robots.end(), __gnu_cxx::compose1(std::bind1st(std::equal_to<uint64_t>(), address), RobotAddress()));
	if (i != robots.end()) {
		unsigned int index = i - robots.begin();
		robots.erase(i);
		signal_robot_removed.emit(index);
	}
}

void Config::RobotSet::replace(uint64_t old_address, uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name) {
	std::vector<RobotInfo>::iterator i = std::find_if(robots.begin(), robots.end(), __gnu_cxx::compose1(std::bind1st(std::equal_to<uint64_t>(), old_address), RobotAddress()));
	assert(i != robots.end());
	assert(address == i->address || !contains_address(address));
	assert((yellow == i->yellow && pattern_index == i->pattern_index) || !contains_pattern(yellow, pattern_index));
	assert(!name.empty());
	assert(name == i->name || !contains_name(name));
	i->address = address;
	i->yellow = yellow;
	i->pattern_index = pattern_index;
	i->name = name;
	signal_robot_replaced.emit(i - robots.begin());
}

void Config::RobotSet::sort_by_address() {
	std::sort(robots.begin(), robots.end(), CompareByAddress());
	signal_sorted.emit();
}

void Config::RobotSet::sort_by_lid() {
	std::sort(robots.begin(), robots.end(), CompareByLid());
	signal_sorted.emit();
}

void Config::RobotSet::sort_by_name() {
	std::sort(robots.begin(), robots.end(), CompareByName());
	signal_sorted.emit();
}

void Config::RobotSet::swap_colours() {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		i->yellow = !i->yellow;
	}
	signal_colours_swapped.emit();
}

void Config::RobotSet::save(std::ostream &ofs) const {
	{
		uint32_t num_robots = robots.size();
		ofs.write(reinterpret_cast<const char *>(&num_robots), 4);
	}
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		ofs.write(reinterpret_cast<const char *>(&i->address), 8);
		{
			uint8_t ui = i->yellow ? 0xFF : 0;
			ofs.write(reinterpret_cast<const char *>(&ui), 1);
		}
		{
			uint32_t ui = i->pattern_index;
			ofs.write(reinterpret_cast<const char *>(&ui), 4);
		}
		const std::string &encoded(i->name);
		{
			uint32_t len = encoded.size();
			ofs.write(reinterpret_cast<const char *>(&len), 4);
			ofs.write(encoded.data(), len);
		}
	}
}

void Config::RobotSet::load_v1(std::istream &ifs) {
	uint32_t num_robots;
	ifs.read(reinterpret_cast<char *>(&num_robots), 4);
	while (num_robots--) {
		uint64_t address;
		ifs.read(reinterpret_cast<char *>(&address), 8);
		bool yellow;
		{
			uint8_t ui;
			ifs.read(reinterpret_cast<char *>(&ui), 1);
			yellow = !!ui;
		}
		unsigned int pattern_index;
		{
			uint32_t ui;
			ifs.read(reinterpret_cast<char *>(&ui), 4);
			pattern_index = ui;
		}
		Glib::ustring name;
		{
			uint32_t len;
			ifs.read(reinterpret_cast<char *>(&len), 4);
			char raw[len];
			ifs.read(raw, len);
			std::string encoded(raw, len);
			name = encoded;
		}
		add(address, yellow, pattern_index, name);
	}
}

void Config::RobotSet::load_v2(std::istream &ifs) {
	load_v1(ifs);
}

