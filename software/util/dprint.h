#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#include <iostream>
#include <iomanip>
#include <glibmm.h>
namespace {
	void dprint(const char *file, unsigned int line, const Glib::ustring &msg) {
		std::cout << file << ':' << line << ": " << msg << '\n';
	}

	Glib::ustring tohex(uint64_t value, unsigned int width) {
		return Glib::ustring::format(std::hex, std::setw(width), std::setfill(L'0'), std::uppercase, value);
	}
}
#define LOG(msg) dprint(__FILE__, __LINE__, msg)
#if DEBUG
#define DPRINT(msg) LOG(msg)
#else
#define DPRINT(msg) do {} while (0)
#endif

#endif

