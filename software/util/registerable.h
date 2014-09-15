#ifndef UTIL_REGISTERABLE_H
#define UTIL_REGISTERABLE_H

#include "util/algorithm.h"
#include "util/noncopyable.h"
#include <map>
#include <stdexcept>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>

/**
 * A generic, templatized class to handle interfaces that should have registerable implementations.
 *
 * \tparam T the type that is actually registerable (that is, the type of the subclass of this class)
 */
template<typename T> class Registerable : public NonCopyable {
	public:
		/**
		 * \brief The type of a map from collation key of object name to object.
		 */
		typedef std::map<std::string, T *> Map;

		/**
		 * \brief Gets a map of all currently-registered objects, keyed by their names’ collation keys.
		 *
		 * \return the map of registered objects of this type
		 */
		static const Map &all() {
			// Flush any created-but-not-yet-inserted-into-the-map objects into the map.
			while (precache()) {
				// Dequeue an object from the list.
				T *obj = dynamic_cast<T *>(precache());
				precache() = obj->precache_next;
				obj->precache_next = nullptr;
				// Check if the name contains a slash, which is illegal.
				if (exists(obj->name_.begin(), obj->name_.end(), 47 /* slash */)) {
					throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Invalid name: %1 (must not contain a slash)", obj->name())));
				}
				// Check if the name duplicates another object of the same type.
				const std::string &key = obj->name().collate_key();
				if (objects().count(key)) {
					throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Duplicate name: %1", obj->name())));
				}
				// Add the object to the map.
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
		 * \brief Constructs a new Registerable object and registers it.
		 *
		 * \param[in] name the name of the new object (encoded in UTF-8)
		 */
		explicit Registerable(const char *name) : name_(name) {
			// Insert into the linked list of preconstructed objects.
			precache_next = precache();
			precache() = this;
		}

		/**
		 * \brief Destroys a Registerable object. Does not unregister it, because we might have some global object destruction order issues.
		 */
		virtual ~Registerable();

	private:
		/**
		 * \brief The object’s name.
		 */
		Glib::ustring name_;

		/**
		 * \brief The next object in the linked list of objects that have been registered but not yet inserted into the main map.
		 */
		Registerable<T> *precache_next;

		/**
		 * \brief Returns the head of the linked list of objects that have been registered but not yet inserted into the main map.
		 *
		 * Such a list is necessary because Registerable objects are constructed globally before main starts running.
		 * This means that, were they added to the map immediately, their collation key would change when main sets the application locale, breaking the map.
		 *
		 * \return a reference to the list head pointer
		 */
		static Registerable<T> * &precache() {
			static Registerable<T> *first;
			return first;
		}

		/**
		 * \brief Returns the main map object.
		 *
		 * \return the map
		 */
		static Map &objects() {
			static Map m;
			return m;
		}
};



template<typename T> Registerable<T>::~Registerable() = default;

#endif

