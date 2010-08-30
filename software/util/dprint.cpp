#include "util/dprint.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

void log_impl(const char *file, unsigned int line, const Glib::ustring &msg, unsigned int level) {
	std::time_t stamp;
	std::time(&stamp);
	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "%F %T", std::localtime(&stamp));
	const char *level_name;
	switch (level) {
		case LOG_LEVEL_DEBUG:
			level_name = "DEBUG";
			break;

		case LOG_LEVEL_INFO:
			level_name = "INFO";
			break;

		case LOG_LEVEL_WARN:
			level_name = "WARN";
			break;

		case LOG_LEVEL_ERROR:
			level_name = "ERROR";
			break;

		default:
			level_name = "UNKNOWN LEVEL";
			break;
	}
	if (level >= LOG_LEVEL_INFO) {
		std::cout << level_name << " [" << buffer << "] [" << file << ':' << line << "] " << msg << '\n';
	}
}

