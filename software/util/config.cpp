#include "util/config.h"
#include "util/exception.h"
#include <fstream>
#include <functional>
#include <glibmm.h>
#include <libgen.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>

using namespace std::placeholders;

namespace {
	/**
	 * \return The parser.
	 */
	xmlpp::DomParser &parser() {
		static xmlpp::DomParser instance;
		return instance;
	}

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
	std::string get_config_filename() {
		return get_bin_directory() + "/../config.xml";
	}

	/**
	 * \brief Removes all text nodes that contain only whitespace.
	 *
	 * \param[in] elt the element from which text nodes should be clean.
	 */
	void clean_whitespace(xmlpp::Element *e) {
		const xmlpp::Node::NodeList &children = e->get_children();
		std::vector<xmlpp::Node *> to_remove;
		for (auto i = children.begin(), iend = children.end(); i != iend; ++i) {
			xmlpp::TextNode *t = dynamic_cast<xmlpp::TextNode *>(*i);
			if (t) {
				const Glib::ustring &text = t->get_content();
				bool found = false;
				for (auto j = text.begin(), jend = text.end(); !found && j != jend; ++j) {
					if (!(*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r')) {
						found = true;
					}
				}
				if (!found) {
					to_remove.push_back(t);
				}
			}
		}
		std::for_each(to_remove.begin(), to_remove.end(), std::bind(std::mem_fn(&xmlpp::Element::remove_child), e, _1));
	}
}

void Config::load() {
	std::ifstream ifs;
	ifs.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
	ifs.open(get_config_filename().c_str(), std::ifstream::in | std::ifstream::binary);
	ifs.exceptions(std::ifstream::goodbit);
	parser().set_validate();
	parser().set_substitute_entities();
	parser().parse_stream(ifs);
	const xmlpp::Document *document = parser().get_document();
	xmlpp::Element *root_elt = document->get_root_node();
	if (root_elt->get_name() != "thunderbots") {
		throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("Malformed config.xml (expected root element of type thunderbots, found %1)", root_elt->get_name())));
	}
	clean_whitespace(root_elt);
}

void Config::save() {
	xmlpp::Document *doc = parser().get_document();
	std::ofstream ofs;
	ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	ofs.open(get_config_filename().c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	doc->write_to_stream_formatted(ofs);
}

xmlpp::Element *Config::params() {
	xmlpp::Document *doc = parser().get_document();
	xmlpp::Element *root_elt = doc->get_root_node();
	const xmlpp::Node::NodeList &params_elts = root_elt->get_children("params");
	if (params_elts.size() != 1) {
		throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("Malformed config.xml (expected exactly one params element, found %1)", params_elts.size())));
	}
	return dynamic_cast<xmlpp::Element *>(*params_elts.begin());
}

