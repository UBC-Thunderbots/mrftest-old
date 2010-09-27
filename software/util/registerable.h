#ifndef UTIL_REGISTERABLE_H
#define UTIL_REGISTERABLE_H

#include "util/noncopyable.h"
#include <glibmm.h>
#include <map>
#include <stdexcept>
#include <vector>

/**
 * A generic, templatized class to handle interfaces that should have registerable implementations.
 *
 * \tparam T the type that is actually registerable (that is, the type of the subclass of this class).
 */
template<typename T>
class Registerable : public NonCopyable {
	public:
		/**
		 * The type of a map from collation key of object name to object.
		 */
		typedef std::map<std::string, T *> Map;

		/**
		 * Gets a map of all currently-registered objects, keyed by their names' collation keys.
		 *
		 * \return the map of registered objects of this type.
		 */
		static const Map &all() {
			typedef typename std::vector<Registerable<T> *>::const_iterator iter_type;
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

		/**
		 * The name of this object.
		 */
		const Glib::ustring name;

	protected:
		/**
		 * Constructs a new Registerable object and registers it.
		 *
		 * \param[in] name the name of the new object.
		 */
		Registerable(const Glib::ustring &name) : name(name) {
			precache().push_back(this);
		}

		/**
		 * Destroys a Registerable object. Does not unregister it, because we might have some global object destruction order issues.
		 */
		virtual ~Registerable() {
		}

	private:
		static std::vector<Registerable<T> *> &precache() {
			static std::vector<Registerable<T> *> v;
			return v;
		}

		static Map &objects() {
			static Map m;
			return m;
		}
};

#endif

