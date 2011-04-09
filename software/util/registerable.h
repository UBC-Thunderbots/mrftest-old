#ifndef UTIL_REGISTERABLE_H
#define UTIL_REGISTERABLE_H

#include "util/noncopyable.h"
#include <glibmm.h>
#include <map>
#include <stdexcept>

/**
 * A generic, templatized class to handle interfaces that should have registerable implementations.
 *
 * \tparam T the type that is actually registerable (that is, the type of the subclass of this class).
 */
template<typename T> class Registerable : public NonCopyable {
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
			while (precache()) {
				T *obj = dynamic_cast<T *>(precache());
				precache() = obj->precache_next;
				obj->precache_next = 0;
				obj->name_ = Glib::locale_to_utf8(obj->name_raw);
				for (Glib::ustring::const_iterator i = obj->name_.begin(), iend = obj->name_.end(); i != iend; ++i) {
					if (*i == '/') {
						throw std::invalid_argument("Invalid name: " + obj->name() + " (must not contain a slash)");
					}
				}
				const std::string &key = obj->name().collate_key();
				if (objects().count(key)) {
					throw std::invalid_argument("Duplicate name: " + obj->name());
				}
				objects()[key] = obj;
			}
			return objects();
		}

		/**
		 * The name of this object.
		 */
		const Glib::ustring &name() const {
			return name_;
		}

	protected:
		/**
		 * Constructs a new Registerable object and registers it.
		 *
		 * \param[in] name the name of the new object.
		 */
		Registerable(const char *name) : name_raw(name) {
			precache_next = precache();
			precache() = this;
		}

		/**
		 * Destroys a Registerable object. Does not unregister it, because we might have some global object destruction order issues.
		 */
		virtual ~Registerable() {
		}

	private:
		const char *const name_raw;
		Glib::ustring name_;
		Registerable<T> *precache_next;

		static Registerable<T> * &precache() {
			static Registerable<T> *first;
			return first;
		}

		static Map &objects() {
			static Map m;
			return m;
		}
};

#endif

