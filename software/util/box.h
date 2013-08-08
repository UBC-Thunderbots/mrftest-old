#ifndef UTIL_BOX_H
#define UTIL_BOX_H

#include "util/box_ptr.h"
#include "util/noncopyable.h"
#include <cassert>
#include <cstring>

/**
 * \brief A container which can hold either one or zero instances of an object.
 *
 * \tparam T the type of object to hold
 */
template<typename T> class Box : public NonCopyable {
	public:
		/**
		 * \brief The type of object in the box.
		 */
		typedef T Value;

		/**
		 * \brief Constructs an empty box.
		 */
		Box();

		/**
		 * \brief Destroys the box, destroying its contents as well if present.
		 */
		~Box();

		/**
		 * \brief Checks whether the box is full.
		 *
		 * \return \c true if the box is full, or \c false if empty
		 */
		explicit operator bool() const;

		/**
		 * \brief Returns the object contained in the box.
		 *
		 * This function must only be called when the box is full.
		 *
		 * \return the object
		 */
		T &value();

		/**
		 * \brief Returns the object contained in the box.
		 *
		 * This function must only be called when the box is full.
		 *
		 * \return the object
		 */
		const T &value() const;

		/**
		 * \brief Constructs a pointer to the box, through which the contained object can be accessed when present.
		 *
		 * \return the pointer
		 */
		BoxPtr<T> ptr();

		/**
		 * \brief Constructs a pointer to the box, through which the contained object can be accessed when present.
		 *
		 * \return the pointer
		 */
		BoxPtr<const T> ptr() const;

		/**
		 * \brief Constructs an object in the box.
		 *
		 * If the box is already full, the object is destroyed then recreated.
		 *
		 * \tparam Args the types of the parameters to T's constructor.
		 *
		 * \param[in] args the parameters to pass to the object's constructor.
		 */
		template<typename ... Args> void create(Args ... args);

		/**
		 * \brief Empties the box.
		 *
		 * If the box is already empty, no action is taken.
		 */
		void destroy();

	private:
		union Union {
			Value value;

			Union();
			~Union();
		};

		bool valid;
		Union data;
};



template<typename T> Box<T>::Box() : valid(false) {
}

template<typename T> Box<T>::~Box() {
	destroy();
}

template<typename T> Box<T>::operator bool() const {
	return valid;
}

template<typename T> T &Box<T>::value() {
	assert(valid);
	return data.value;
}

template<typename T> const T &Box<T>::value() const {
	assert(valid);
	return data.value;
}

template<typename T> BoxPtr<T> Box<T>::ptr() {
	return BoxPtr<T>(&data.value, &valid);
}

template<typename T> BoxPtr<const T> Box<T>::ptr() const {
	return BoxPtr<const T>(&data.value, &valid);
}

template<typename T> template<typename ... Args> void Box<T>::create(Args ... args) {
	destroy();
	new(&data.value) T(args...);
	valid = true;
}

template<typename T> void Box<T>::destroy() {
	if (valid) {
		data.value.~T();
		valid = false;
		// By zeroing memory, we assure that future attempts to invoke virtual functions on the object will segfault when dereferencing the vtable pointer.
		std::memset(static_cast<void *>(&data.value), 0, sizeof(data.value));
	}
}



template<typename T> Box<T>::Union::Union() {
	std::memset(static_cast<void *>(&value), 0, sizeof(value));
}

template<typename T> Box<T>::Union::~Union() {
}

#endif

