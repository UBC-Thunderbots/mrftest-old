#ifndef UTIL_OBJECTSTORE_H
#define UTIL_OBJECTSTORE_H

#include "util/noncopyable.h"
#include <map>
#include <memory>
#include <typeinfo>

/**
 * \brief An ObjectStore allows multiple clients to store arbitrary objects attached to another object,
 * without the holder knowing a priori the details of the stored objects,
 * and without multiple clients overwriting each others' stored objects.
 */
class ObjectStore : public NonCopyable {
	public:
		/**
		 * \brief The type of an element in an ObjectStore.
		 * Clients should subclass this class to add their own data.
		 */
		class Element : public NonCopyable {
			public:
				/**
				 * \brief A pointer to an Element.
				 */
				typedef std::shared_ptr<Element> Ptr;

				/**
				 * \brief Destroys an Element.
				 */
				virtual ~Element();
		};

		/**
		 * \brief Constructs a new ObjectStore containing no objects.
		 */
		ObjectStore();

		/**
		 * \brief Fetches an object from the ObjectStore, creating it if it does not yet exist.
		 *
		 * \param[in] tid the key identifying the specific client (generally <code>typeid(*this)</code> in the client).
		 *
		 * \return the object associated with the client.
		 */
		Element::Ptr &operator[](const std::type_info &tid);

		/**
		 * \brief Removes all elements from the ObjectStore.
		 */
		void clear();

	private:
		std::map<const std::type_info *, Element::Ptr, bool (*)(const std::type_info *, const std::type_info *)> data;
};

#endif

