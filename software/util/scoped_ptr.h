#ifndef UTIL_SCOPED_PTR_H
#define UTIL_SCOPED_PTR_H

#include "util/noncopyable.h"

/**
 * A smart pointer that can never be copied, but can be repointed to a new
 * target.
 */
template<typename T>
class ScopedPtr : public NonCopyable {
	public:
		/**
		 * Constructs a new ScopedPtr with no object.
		 */
		ScopedPtr() : obj(0) {
		}

		/**
		 * Constructs a new ScopedPtr pointing to an object.
		 *
		 * \param o the new object to point to.
		 */
		ScopedPtr(T *o) : obj(o) {
		}

		/**
		 * Destroys the pointed-to object.
		 */
		~ScopedPtr() {
			delete obj;
		}

		/**
		 * Resets the ScopedPtr to point at another object.
		 *
		 * \param o the new object to point to.
		 */
		void reset(T *o) {
			delete obj;
			obj = o;
		}

		/**
		 * \return the contained object.
		 */
		T *operator->() const {
			return obj;
		}

		/**
		 * \return the contained object.
		 */
		T &ref() const {
			return *obj;
		}

		/**
		 * \return \c true if the pointer is non-null, or \c false if the
		 * pointer is null.
		 */
		operator bool() const {
			return !!obj;
		}

	private:
		T *obj;
};

#endif

