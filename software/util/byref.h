#ifndef UTIL_BYREF_H
#define UTIL_BYREF_H

#include "util/noncopyable.h"

//
// An object that should be passed around by means of a Glib::RefPtr<> rather
// than by copying.
//
class byref : public noncopyable {
	public:
		//
		// Adds one to the object's reference count. This should only be called
		// by Glib::RefPtr, not by application code.
		//
		virtual void reference() {
			refs_++;
		}

		//
		// Subtracts one from the object's reference count. This should only be
		// called by Glib::RefPtr, not by application code.
		//
		virtual void unreference() {
			if (!--refs_)
				delete this;
		}

		//
		// Returns the reference count of the object. This can be used to check
		// for leaking references at the expected point of destruction.
		//
		unsigned int refs() const {
			return refs_;
		}

	protected:
		//
		// Constructs a new byref. The object is assumed to have one reference.
		//
		byref() : refs_(1) {
		}

		//
		// Destroys a byref. This is here even though it doesn't do anything
		// because it forces destructors all the way down the inheritance
		// hierarchy to be virtual, which ensures that when a reference-counted
		// object loses its last pointer, the "delete this" in unreference()
		// invokes the correct destructor.
		//
		virtual ~byref() {
		}

	private:
		//
		// The reference count of the object.
		//
		unsigned int refs_;
};

#endif

