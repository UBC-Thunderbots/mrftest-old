#include "util/config.h"
#include <string>
#include <stdexcept>
#include <cassert>
#include <cstdio>
#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

namespace {
	xmlpp::DomParser *parser = 0;
	Glib::ustring filename;
	sigc::connection timer_connection;
	bool is_dirty = false;

	bool timeout() {
		config::force_save();
		return false;
	}
}

xmlpp::Document *config::get() {
	if (!parser) {
		parser = new xmlpp::DomParser;
		try {
			parser->set_substitute_entities(true);
			const std::string &base = Glib::get_user_config_dir();
			const std::string &sub = base + "/thunderbots";
			mkdir(sub.c_str(), 0777);
			const std::string &file = sub + "/config.xml";
			int fd = open(file.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
			if (fd >= 0) {
				const std::string tmpl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<thunderbots />\n");
				if (write(fd, tmpl.data(), tmpl.size()) != static_cast<ssize_t>(tmpl.size())) {
					close(fd);
					std::remove(file.c_str());
					throw std::runtime_error("Cannot create empty config file");
				}
				close(fd);
			}
			filename = Glib::locale_to_utf8(file);
			parser->parse_file(filename);
		} catch (...) {
			delete parser;
			parser = 0;
			throw;
		}
	}

	return parser->get_document();
}

void config::dirty() {
	is_dirty = true;
	if (!timer_connection)
		timer_connection = Glib::signal_timeout().connect_seconds(&timeout, 3);
}

void config::force_save() {
	timer_connection.disconnect();
	if (is_dirty) {
		assert(parser);
		xmlpp::Document *doc = parser->get_document();
		assert(doc);
		doc->write_to_file_formatted(filename);
		is_dirty = false;
	}
}

