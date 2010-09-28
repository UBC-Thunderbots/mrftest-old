#include "util/config.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <glibmm.h>
#include <iomanip>
#include <libgen.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <ext/functional>
#include <libxml++/libxml++.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	/**
	 * A unary functor that accepts a RobotInfo and returns its address.
	 */
	class RobotAddress : public std::unary_function<const Config::RobotInfo &, uint64_t> {
		public:
			/**
			 * Executes the functor.
			 *
			 * \param[in] bot the RobotInfo to examine.
			 *
			 * \return the address of the robot.
			 */
			uint64_t operator()(const Config::RobotInfo &bot) const {
				return bot.address;
			}
	};

	/**
	 * A binary predicate that compares two robots by their 64-bit addresses.
	 */
	class CompareByAddress : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			/**
			 * Executes the functor.
			 *
			 * \param[in] x the first RobotInfo to examine.
			 *
			 * \param[in] y the second RobotInfo to examine.
			 *
			 * \return \c true if \p x has an address less than that of \c y, or \c false if not.
			 */
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				return x.address < y.address;
			}
	};

	/**
	 * A binary predicate that compares two robots by their lid patterns.
	 */
	class CompareByLid : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			/**
			 * Executes the functor.
			 *
			 * \param[in] x the first RobotInfo to examine.
			 *
			 * \param[in] y the second RobotInfo to examine.
			 *
			 * \return \c true if \p x comes before \p y when ordered by lid patterns, or \c false if not.
			 */
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				return x.pattern_index < y.pattern_index;
			}
	};

	/**
	 * A binary predicate that compares two robots by their names.
	 */
	class CompareByName : public std::binary_function<const Config::RobotInfo &, const Config::RobotInfo &, bool> {
		public:
			/**
			 * Executes the functor.
			 *
			 * \param[in] x the first RobotInfo to examine.
			 *
			 * \param[in] y the second RobotInfo to examine.
			 *
			 * \return \c true if the name of \p x comes lexicographically before the name of \c y, or \c false if not.
			 */
			bool operator()(const Config::RobotInfo &x, const Config::RobotInfo &y) {
				return x.name < y.name;
			}
	};

	/**
	 * \return the directory in which the running executable is stored.
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
	 * \return the filename of the config file.
	 */
	std::string get_filename() {
		return get_bin_directory() + "/../config.xml";
	}

	/**
	 * Finds the only child element of a parent with a given name.
	 *
	 * \param[in] parent the parent element to search.
	 *
	 * \param[in] name the name of the child to look for.
	 *
	 * \return the child element, or null if no (or more than one) child had the requested name.
	 */
	xmlpp::Element *find_child_element(const xmlpp::Element *parent, const Glib::ustring &name) {
		const xmlpp::Node::NodeList &children = parent->get_children(name);
		xmlpp::Element *found = 0;
		for (xmlpp::Node::NodeList::const_iterator i = children.begin(), iend = children.end(); i != iend; ++i) {
			xmlpp::Element *elt = dynamic_cast<xmlpp::Element *>(*i);
			if (elt) {
				if (found) {
					return 0;
				}
				found = elt;
			}
		}
		return found;
	}
}

Config::Config() : channel_(0x0E) {
	// Load file.
	std::ifstream ifs;
	try {
		ifs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		const std::string &filename = get_filename();
		ifs.open(filename.c_str(), std::ios::in);
	} catch (const std::ifstream::failure &exp) {
		// Swallow.
		return;
	}

	ifs.exceptions(std::ios_base::goodbit);
	xmlpp::DomParser parser;
	parser.set_substitute_entities(true);
	parser.parse_stream(ifs);
	if (parser) {
		const xmlpp::Document *document = parser.get_document();
		const xmlpp::Element *root = document->get_root_node();
		if (root) {
			const xmlpp::Element *players = find_child_element(root, "players");
			if (players) {
				robots_.load(players);
			}

			const xmlpp::Element *radio = find_child_element(root, "radio");
			if (radio) {
				const Glib::ustring &channel_string = radio->get_attribute_value("channel");
				std::istringstream iss(channel_string);
				iss >> std::hex >> channel_;
			}

			const xmlpp::Element *params = find_child_element(root, "params");
			if (params) {
				const xmlpp::Node::NodeList &bool_param_list = params->get_children("bool-param");
				for (xmlpp::Node::NodeList::const_iterator i = bool_param_list.begin(), iend = bool_param_list.end(); i != iend; ++i) {
					const xmlpp::Element *bool_param = dynamic_cast<xmlpp::Element *>(*i);
					if (bool_param) {
						const Glib::ustring &name = bool_param->get_attribute_value("name");
						const xmlpp::TextNode *value_node = bool_param->get_child_text();
						if (value_node) {
							const Glib::ustring &value_string = value_node->get_content();
							bool value = value_string == "true";
							bool_params[name] = value;
						}
					}
				}

				const xmlpp::Node::NodeList &int_param_list = params->get_children("int-param");
				for (xmlpp::Node::NodeList::const_iterator i = int_param_list.begin(), iend = int_param_list.end(); i != iend; ++i) {
					const xmlpp::Element *int_param = dynamic_cast<xmlpp::Element *>(*i);
					if (int_param) {
						const Glib::ustring &name = int_param->get_attribute_value("name");
						const xmlpp::TextNode *value_node = int_param->get_child_text();
						if (value_node) {
							const Glib::ustring &value_string = value_node->get_content();
							std::istringstream iss(value_string);
							int value;
							iss >> value;
							int_params[name] = value;
						}
					}
				}

				const xmlpp::Node::NodeList &double_param_list = params->get_children("double-param");
				for (xmlpp::Node::NodeList::const_iterator i = double_param_list.begin(), iend = double_param_list.end(); i != iend; ++i) {
					const xmlpp::Element *double_param = dynamic_cast<xmlpp::Element *>(*i);
					if (double_param) {
						const Glib::ustring &name = double_param->get_attribute_value("name");
						const xmlpp::TextNode *value_node = double_param->get_child_text();
						if (value_node) {
							const Glib::ustring &value_string = value_node->get_content();
							std::istringstream iss(value_string);
							double value;
							iss >> value;
							double_params[name] = value;
						}
					}
				}
			}
		}
	}
}

void Config::save() const {
	// Save file.
	std::ofstream ofs;
	ofs.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	const std::string &filename = get_filename();
	ofs.open(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	xmlpp::Document document;
	xmlpp::Element *root = document.create_root_node("thunderbots");
	root->add_child_text("\n");

	if (robots_.size() > 0) {
		xmlpp::Element *players = root->add_child("players");
		robots_.save(players);
		root->add_child_text("\n");
	}

	xmlpp::Element *radio = root->add_child("radio");
	radio->set_attribute("channel", tohex(channel_, 2));
	root->add_child_text("\n");

	xmlpp::Element *params = root->add_child("params");
	if (!bool_params.empty() || !int_params.empty() || !double_params.empty()) {
		params->add_child_text("\n");
	}
	for (std::map<Glib::ustring, bool>::const_iterator i = bool_params.begin(), iend = bool_params.end(); i != iend; ++i) {
		xmlpp::Element *bool_param = params->add_child("bool-param");
		bool_param->set_attribute("name", i->first);
		bool_param->add_child_text(i->second ? "true" : "false");
		params->add_child_text("\n");
	}
	for (std::map<Glib::ustring, int>::const_iterator i = int_params.begin(), iend = int_params.end(); i != iend; ++i) {
		xmlpp::Element *int_param = params->add_child("int-param");
		int_param->set_attribute("name", i->first);
		std::wostringstream oss;
		oss.imbue(std::locale("C"));
		oss << i->second;
		int_param->add_child_text(Glib::ustring::format(oss.str()));
		params->add_child_text("\n");
	}
	for (std::map<Glib::ustring, double>::const_iterator i = double_params.begin(), iend = double_params.end(); i != iend; ++i) {
		xmlpp::Element *double_param = params->add_child("double-param");
		double_param->set_attribute("name", i->first);
		std::wostringstream oss;
		oss.precision(9);
		oss.imbue(std::locale("C"));
		oss << i->second;
		double_param->add_child_text(Glib::ustring::format(oss.str()));
		params->add_child_text("\n");
	}
	root->add_child_text("\n");

	document.write_to_stream(ofs, "UTF-8");
}

void Config::channel(unsigned int chan) {
	assert(0x0B <= chan && chan <= 0x1A);
	channel_ = chan;
}

const Config::RobotInfo &Config::RobotSet::find(uint64_t address) const {
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->address == address) {
			return *i;
		}
	}
	throw std::runtime_error("Cannot find robot by address!");
}

const Config::RobotInfo &Config::RobotSet::find(const Glib::ustring &name) const {
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->name == name) {
			return *i;
		}
	}
	throw std::runtime_error("Cannot find robot by name!");
}

bool Config::RobotSet::contains_address(uint64_t address) const {
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->address == address) {
			return true;
		}
	}
	return false;
}

bool Config::RobotSet::contains_pattern(unsigned int pattern_index) const {
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->pattern_index == pattern_index) {
			return true;
		}
	}
	return false;
}

bool Config::RobotSet::contains_name(const Glib::ustring &name) const {
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		if (i->name == name) {
			return true;
		}
	}
	return false;
}

void Config::RobotSet::add(uint64_t address, unsigned int pattern_index, const Glib::ustring &name) {
	assert(!contains_address(address));
	assert(!contains_pattern(pattern_index));
	assert(!name.empty());
	assert(!contains_name(name));
	unsigned int index = robots.size();
	robots.push_back(RobotInfo(address, pattern_index, name));
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

void Config::RobotSet::replace(uint64_t old_address, uint64_t address, unsigned int pattern_index, const Glib::ustring &name) {
	std::vector<RobotInfo>::iterator i = std::find_if(robots.begin(), robots.end(), __gnu_cxx::compose1(std::bind1st(std::equal_to<uint64_t>(), old_address), RobotAddress()));
	assert(i != robots.end());
	assert(address == i->address || !contains_address(address));
	assert(pattern_index == i->pattern_index || !contains_pattern(pattern_index));
	assert(!name.empty());
	assert(name == i->name || !contains_name(name));
	i->address = address;
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

void Config::RobotSet::load(const xmlpp::Element *players) {
	const xmlpp::Node::NodeList &player_list = players->get_children("player");
	for (xmlpp::Node::NodeList::const_iterator i = player_list.begin(), iend = player_list.end(); i != iend; ++i) {
		xmlpp::Element *player = dynamic_cast<xmlpp::Element *>(*i);
		if (player) {
			const Glib::ustring &address_string = player->get_attribute_value("address");
			uint64_t address = 0;
			{
				std::istringstream iss(address_string);
				iss >> std::hex >> address;
			}
			const Glib::ustring &pattern_index_string = player->get_attribute_value("pattern");
			unsigned int pattern_index = 0;
			{
				std::istringstream iss(pattern_index_string);
				iss >> pattern_index;
			}
			const Glib::ustring &name = player->get_attribute_value("name");
			add(address, pattern_index, name);
		}
	}
}

void Config::RobotSet::save(xmlpp::Element *players) const {
	players->add_child_text("\n");
	for (std::vector<RobotInfo>::const_iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		xmlpp::Element *player = players->add_child("player");
		player->set_attribute("address", tohex(i->address, 16));
		std::wostringstream oss;
		oss.imbue(std::locale("C"));
		oss << i->pattern_index;
		player->set_attribute("pattern", Glib::ustring::format(oss.str()));
		player->set_attribute("name", i->name);
		players->add_child_text("\n");
	}
}

