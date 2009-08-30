#ifndef UTIL_BYREF_H
#define UTIL_BYREF_H

//
// An object that should be passed around by means of a Glib::RefPtr<> rather
// than by copying.
//
class byref {
	public:
		byref() : refs(1) {
		}

		void reference() {
			refs++;
		}

		void unreference() {
			if (!--refs)
				delete this;
		}

	protected:
		virtual ~byref() {
		}

		byref(const byref &copyref);

		byref &operator=(const byref &assgref);

		friend class Glib::RefPtr<byref>;

		unsigned int refs;
};

#endif

