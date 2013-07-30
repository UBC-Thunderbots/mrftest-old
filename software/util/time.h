#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <chrono>

// GCC <4.7 defines “std::chrono::monotonic_clock” instead of the standards-compliant “std::chrono::steady_clock”.
#ifdef __GNUC__
#if (__GNUC__ == 4) && (__GNUC_MINOR__ < 7)
namespace std {
	namespace chrono {
		typedef monotonic_clock steady_clock;
	}
}
#endif
#endif

#endif

