#include "util/algorithm.h"
#include "util/config.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <ext/functional>
#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	//
	// A unary functor that accepts a robot_info and returns its address.
	//
	class robot_address : public std::unary_function<const config::robot_info &, uint64_t> {
		public:
			uint64_t operator()(const config::robot_info &bot) const {
				return bot.address;
			}
	};
}

config::config() {
	// Find filename.
	const Glib::ustring &config_dir = Glib::get_user_config_dir();
	const std::string &file_path = Glib::filename_from_utf8(config_dir + "/thunderbots/config.dat");

	// Load file.
	std::ifstream ifs;
	try {
		ifs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		ifs.open(file_path.c_str(), std::ios::in | std::ios::binary);
		char signature[8];
		ifs.read(signature, sizeof(signature));
		if (std::equal(signature, signature + 8, "TBOTC001")) {
			load_v1(ifs);
		} else {
			// Unknown version number. Give up.
		}
	} catch (const std::ifstream::failure &exp) {
		// Swallow.
	}
}

void config::save() const {
	// Find filename.
	const Glib::ustring &config_dir = Glib::get_user_config_dir();
	const std::string &parent_dir = Glib::filename_from_utf8(config_dir + "/thunderbots");
	mkdir(parent_dir.c_str(), 0755);
	const std::string &file_path = Glib::filename_from_utf8(config_dir + "/thunderbots/config.dat");

	// Save file.
	std::ofstream ofs;
	ofs.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	ofs.open(file_path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	ofs.write("TBOTC001", 8);
	robots_.save(ofs);
}

void config::load_v1(std::istream &ifs) {
	robots_.load_v1(ifs);
}

const config::robot_info &config::robot_set::find(uint64_t address) const {
	for (typeof(robots.begin()) i(robots.begin()), iend(robots.end()); i != iend; ++i) {
		if (i->address == address) {
			return *i;
		}
	}
	throw std::runtime_error("Cannot find robot by address!");
}

bool config::robot_set::contains_address(uint64_t address) const {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->address == address) {
			return true;
		}
	}
	return false;
}

bool config::robot_set::contains_pattern(bool yellow, unsigned int pattern_index) const {
	for (typeof(robots.begin()) i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->yellow == yellow && i->pattern_index == pattern_index) {
			return true;
		}
	}
	return false;
}

void config::robot_set::add(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name) {
	assert(!contains_address(address));
	assert(!contains_pattern(yellow, pattern_index));
	unsigned int index = robots.size();
	robots.push_back(robot_info(address, yellow, pattern_index, name));
	signal_robot_added.emit(index);
}

void config::robot_set::remove(uint64_t address) {
	std::vector<robot_info>::iterator i = std::find_if(robots.begin(), robots.end(), __gnu_cxx::compose1(std::bind1st(std::equal_to<uint64_t>(), address), robot_address()));
	if (i != robots.end()) {
		unsigned int index = i - robots.begin();
		robots.erase(i);
		signal_robot_removed.emit(index);
	}
}

void config::robot_set::save(std::ostream &ofs) const {
	{
		uint32_t num_robots = robots.size();
		ofs.write(reinterpret_cast<const char *>(&num_robots), 4);
	}
	for (std::vector<robot_info>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
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

void config::robot_set::load_v1(std::istream &ifs) {
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

