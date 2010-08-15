#ifndef XBEE_SHARED_FRONTEND_NUMBER_ALLOCATOR_H
#define XBEE_SHARED_FRONTEND_NUMBER_ALLOCATOR_H

#include "util/noncopyable.h"
#include <cassert>
#include <queue>
#include <sigc++/sigc++.h>

/**
 * An object that manages allocating index numbers.
 */
template<typename T>
class NumberAllocator : public NonCopyable, public sigc::trackable {
	public:
		/**
		 * Constructs a new allocator.
		 *
		 * \param[in] minimum the lower limit on the values to allocate.
		 *
		 * \param[in] maximum the upper limit on the values to allocate.
		 */
		NumberAllocator(T minimum, T maximum) {
			// Do not optimize this to a "for" loop.
			// It is NOT equivalent.
			// It handles the case of maximum == FOO_MAX properly.
			T i = minimum;
			do {
				q.push(i);
			} while (i++ != maximum);
		}

		/**
		 * Allocates a value.
		 *
		 * \return the newly-allocated value.
		 */
		T alloc() {
			assert(available());
			T t = q.front();
			q.pop();
			return t;
		}

		/**
		 * Frees a value.
		 *
		 * \param[in] t the value to free.
		 */
		void free(T t) {
			q.push(t);
		}

		/**
		 * Checks whether any values are available to allocate.
		 *
		 * \return \c true if there are available values, or \c false if not.
		 */
		bool available() {
			return !q.empty();
		}

	private:
		std::queue<T> q;
};

#endif

