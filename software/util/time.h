#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <ctime>
#include <stdexcept>

namespace {
	/**
	 * Gets the current time into a timespec.
	 * \param result the location at which to store the current time
	 */
	void timespec_now(timespec *result) {
		if (clock_gettime(CLOCK_MONOTONIC, result) < 0) {
			throw std::runtime_error("Cannot get monotonic time.");
		}
	}

	/**
	 * Gets the current time into a timespec.
	 * \param result the location at which to store the current time
	 */
	void timespec_now(timespec &result) {
		timespec_now(&result);
	}

	/**
	 * Subtracts a pair of timespecs.
	 * \param ts1 the first timespec
	 * \param ts2 the second timespec
	 * \param result a location at which to store the value of ts1 - ts2
	 */
	void timespec_sub(const timespec &ts1, const timespec &ts2, timespec &result) {
		if (ts1.tv_nsec >= ts2.tv_nsec) {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec;
			result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
		} else {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
			result.tv_nsec = ts1.tv_nsec + 1000000000L - ts2.tv_nsec;
		}
	}

	/**
	 * Compares a pair of timespecs.
	 * \param ts1 the first timespec
	 * \param ts2 the second timespec
	 * \return A positive value if ts1 > ts2, a negative value if ts1 < ts2, or
	 * zero of ts1 = ts2.
	 */
	int timespec_cmp(const timespec &ts1, const timespec &ts2) {
		if (ts1.tv_sec != ts2.tv_sec) {
			return ts1.tv_sec > ts2.tv_sec ? 1 : -1;
		} else if (ts1.tv_nsec != ts2.tv_nsec) {
			return ts1.tv_nsec > ts2.tv_nsec ? 1 : -1;
		} else {
			return 0;
		}
	}

	/**
	 * Converts a timespec to a double-precision count of seconds.
	 * \param ts the timespec to convert
	 * \return The number of seconds represented by ts
	 */
	double timespec_to_double(const timespec &ts) {
		return ts.tv_sec + ts.tv_nsec / 1000000000.0;
	}
}

#endif

