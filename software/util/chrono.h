#ifndef UTIL_CHRONO_H
#define UTIL_CHRONO_H

#include <chrono>

// This is a workaround for an incorrect implementation of the C++ standard library in GCC prior to version 4.7.
// This file can be deleted and references to it replaced with <chrono> once GCC 4.7 is commonly available.
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
namespace std {
	namespace chrono {
		typedef std::chrono::monotonic_clock steady_clock;
	}
}
#endif

#endif

