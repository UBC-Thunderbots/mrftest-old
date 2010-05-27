#ifndef UTIL_ALGORITHM_H
#define UTIL_ALGORITHM_H

#include <algorithm>

namespace {
	//
	// Returns true if an element exists within a range, or false if not.
	//
	template<typename Titer, typename Telem>
	bool exists(Titer begin, Titer end, const Telem &elem) {
		return std::find(begin, end, elem) != end;
	}

	//
	// Returns true if an element exists within a range that satisfies a
	// predicate, or false if not.
	//
	template<typename Titer, typename Tpred>
	bool exists_if(Titer begin, Titer end, Tpred pred) {
		return std::find_if(begin, end, pred) != end;
	}

	//
	// Clamps a value to fall within a given range.
	//
	template<typename T>
	T clamp(T value, T lower, T upper) {
		return std::min(std::max(value, lower), upper);
	}
}

#endif

