#ifndef UTIL_OBJECTSTORE_H
#define UTIL_OBJECTSTORE_H

#include "util/byref.h"
#include "util/noncopyable.h"
#include <map>
#include <typeinfo>

/**
 * An ObjectStore allows multiple clients to store arbitrary objects attached to another object,
 * without the holder knowing a priori the details of the stored objects,
 * and without multiple clients overwriting each others' stored objects.
 */
class ObjectStore : public NonCopyable {
	public:
		/**
		 * The type of an element in an ObjectStore.
		 * Clients should subclass this class to add their own data.
		 */
		class Element : public ByRef {
			public:
				/**
				 * A pointer to an Element.
				 */
				typedef RefPtr<Element> Ptr;

				/**
				 * Constructs a new Element.
				 */
				Element();

				/**
				 * Destroys the Element.
				 */
				~Element();
		};

		/**
		 * Constructs a new ObjectStore containing no objects.
		 */
		ObjectStore();

		/**
		 * Destroys the ObjectStore and the objects it contains.
		 */
		~ObjectStore();

		/**
		 * Fetches an object from the ObjectStore, creating it if it does not yet exist.
		 *
		 * \param[in] k the key identifying the specific client (generally <code>typeid(*this)</code> in the client).
		 *
		 * \return the object associated with the client.
		 */
		Element::Ptr &operator[](const std::type_info &tid);

	private:
		std::map<const std::type_info *, Element::Ptr, bool (*)(const std::type_info *, const std::type_info *)> data;
};

#endif

