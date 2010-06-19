#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#include <iomanip>
#include <stdint.h>
#include <glibmm.h>

void log_impl(const char *file, unsigned int line, const Glib::ustring &msg, unsigned int level);

namespace {
	Glib::ustring tohex(uintmax_t value, unsigned int width) {
		return Glib::ustring::format(std::hex, std::setw(width), std::setfill(L'0'), std::uppercase, value);
	}
}

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3

#define LOG_DEBUG(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_DEBUG)
#define LOG_INFO(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_INFO)
#define LOG_WARN(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_WARN)
#define LOG_ERROR(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_ERROR)

#endif

