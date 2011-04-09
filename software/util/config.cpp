#include "util/config.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/exception.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <glibmm.h>
#include <iomanip>
#include <libgen.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <ext/functional>
#include <libxml++/libxml++.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
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
			throw SystemError("readlink(/proc/self/exe)", errno);
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

Config::Config() : out_channel_(0x0E), in_channel_(0x0F) {
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
			const xmlpp::Element *radio = find_child_element(root, "radio");
			if (radio) {
				const Glib::ustring &out_channel_string = radio->get_attribute_value("out-channel");
				std::istringstream out_iss(out_channel_string);
				out_iss >> std::hex >> out_channel_;

				const Glib::ustring &in_channel_string = radio->get_attribute_value("in-channel");
				std::istringstream in_iss(in_channel_string);
				in_iss >> std::hex >> in_channel_;
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

	xmlpp::Element *radio = root->add_child("radio");
	radio->set_attribute("out-channel", tohex(out_channel_, 2));
	radio->set_attribute("in-channel", tohex(in_channel_, 2));
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

void Config::channels(unsigned int out, unsigned int in) {
	assert(0x0B <= out && out <= 0x1A);
	assert(0x0B <= in && in <= 0x1A);
	out_channel_ = out;
	in_channel_ = in;
}

