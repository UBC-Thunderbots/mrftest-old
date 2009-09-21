#ifndef UTIL_BYREF_H
#define UTIL_BYREF_H

#include "util/noncopyable.h"

//
// An object that should be passed around by means of a Glib::RefPtr<> rather
// than by copying.
//
class byref : public virtual noncopyable {
	public:
		//
		// Adds one to the object's reference count. This should only be called
		// by Glib::RefPtr, not by application code.
		//
		void reference() {
			refs++;
		}

		//
		// Subtracts one from the object's reference count. This should only be
		// called by Glib::RefPtr, not by application code.
		//
		void unreference() {
			if (!--refs)
				delete this;
		}

	protected:
		//
		// Constructs a new byref. The object is assumed to have one reference.
		//
		byref() : refs(1) {
		}

		//
		// Destroys a byref.
		//
		virtual ~byref() {
		}

	private:
		//
		// The reference count of the object.
		//
		unsigned int refs;
};

#endif

