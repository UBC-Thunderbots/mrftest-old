#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include "util/noncopyable.h"
#include <glibmm.h>

using Glib::RefPtr;

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

/**
 * An object that should be passed around by means of a RefPtr rather than by
 * copying.
 */
class ByRef : public NonCopyable {
	public:
		/**
		 * Adds one to the object's reference count. This should only be called
		 * by RefPtr, not by application code.
		 */
		void reference() {
			++refs_;
		}

		/**
		 * Subtracts one from the object's reference count. This should only be
		 * called by RefPtr, not by application code.
		 */
		void unreference() {
			if (!--refs_) {
				destroy();
			}
		}

		/**
		 * \return the reference count of the object
		 */
		unsigned int refs() const;

	protected:
		/**
		 * Constructs a new ByRef. The object is assumed to have one reference.
		 */
		ByRef();

		/**
		 * Destroys a ByRef. This is here even though it doesn't do anything
		 * because it forces destructors all the way down the inheritance
		 * hierarchy to be virtual, which ensures that when a reference-counted
		 * object loses its last pointer, the \c delete \c this in unreference()
		 * invokes the correct destructor.
		 */
		virtual ~ByRef();

		/**
		 * Destroys the current object.
		 */
		void destroy();

	private:
		/**
		 * The reference count of the object.
		 */
		unsigned int refs_;
};

#endif

