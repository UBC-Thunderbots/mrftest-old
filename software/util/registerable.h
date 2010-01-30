#ifndef UTIL_REGISTERABLE_H
#define UTIL_REGISTERABLE_H

#include "util/noncopyable.h"
#include <map>
#include <stdexcept>
#include <vector>
#include <glibmm.h>

//
// A generic, templatized class to handle interfaces that should have
// registerable implementations.
//
template<typename T>
class registerable : public noncopyable {
	public:
		//
		// The type of a map from collation key of object name to object.
		//
		typedef std::map<std::string, T *> map_type;

		//
		// Gets a map of all currently-registered objects, keyed by their names'
		// collation keys.
		//
		static const map_type &all() {
			typedef typename std::vector<registerable<T> *>::const_iterator iter_type;
			for (iter_type i = precache().begin(), iend = precache().end(); i != iend; ++i) {
				T *obj = dynamic_cast<T *>(*i);
				const std::string &key = obj->name.collate_key();
				if (objects().count(key)) {
					throw std::runtime_error("Duplicate name: " + obj->name);
				}
				objects()[key] = obj;
			}
			precache().clear();
			return objects();
		}

		//
		// The name of this object.
		//
		const Glib::ustring name;

	protected:
		//
		// Constructs a new registerable object and registers it.
		//
		registerable(const Glib::ustring &the_name) : name(the_name) {
			precache().push_back(this);
		}

		//
		// Destroys a registerable object. Does not unregister it, because we
		// might have some global object destruction order issues.
		//
		virtual ~registerable() {
		}

	private:
		static std::vector<registerable<T> *> &precache() {
			static std::vector<registerable<T> *> v;
			return v;
		}

		static map_type &objects() {
			static map_type m;
			return m;
		}
};

#endif

