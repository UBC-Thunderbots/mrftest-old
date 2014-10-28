#ifndef UTIL_OBJECT_STORE_H
#define UTIL_OBJECT_STORE_H

#include "util/noncopyable.h"
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

/**
 * \brief An ObjectStore allows multiple clients to store arbitrary objects attached to another object, without the holder knowing a priori the details of the stored objects, and without multiple clients overwriting each others’ stored objects.
 */
class ObjectStore final : public NonCopyable {
	public:
		/**
		 * \brief The type of an element in an ObjectStore.
		 *
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
		 * \brief Fetches an object from the ObjectStore, creating it if it does not yet exist.
		 *
		 * \param[in] tid the key identifying the specific client (generally <code>typeid(*this)</code> in the client)
		 *
		 * \return the object associated with the client
		 */
		Element::Ptr &operator[](const std::type_info &tid);

		/**
		 * \brief Removes all elements from the ObjectStore.
		 */
		void clear();

	private:
		std::unordered_map<std::type_index, Element::Ptr> data;
};

#endif

