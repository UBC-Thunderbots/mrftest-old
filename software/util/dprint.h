#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#if DEBUG
namespace {
	void dprint(const char *file, unsigned int line, const Glib::ustring &msg) {
		std::cout << file << ':' << line << ": " << msg << '\n';
	}
}
#define DPRINT(msg) dprint(__FILE__, __LINE__, msg)
#else
#define DPRINT(msg) do {} while (0)
#endif

#endif

