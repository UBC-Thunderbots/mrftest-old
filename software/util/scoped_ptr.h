#ifndef UTIL_SCOPED_PTR_H
#define UTIL_SCOPED_PTR_H

#include "util/noncopyable.h"

/**
 * A smart pointer that can never be copied, but can be repointed to a new
 * target.
 */
template<typename T>
class scoped_ptr : public noncopyable {
	public:
		/**
		 * Constructs a new scoped_ptr with no object.
		 */
		scoped_ptr() : obj(0) {
		}

		/**
		 * Constructs a new scoped_ptr pointing to an object.
		 *
		 * \param o the new object to point to.
		 */
		scoped_ptr(T *o) : obj(o) {
		}

		/**
		 * Destroys the pointed-to object.
		 */
		~scoped_ptr() {
			delete obj;
		}

		/**
		 * Resets the scoped_ptr to point at another object.
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

