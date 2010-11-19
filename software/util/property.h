#ifndef UTIL_PROPERTY_H
#define UTIL_PROPERTY_H

#include "util/noncopyable.h"
#include <sigc++/sigc++.h>

/**
 * A variable holding a value of some type, with the ability to notify listeners when the value changes.
 *
 * \tparam T the type of value to hold.
 */
template<typename T> class Property : public NonCopyable {
	public:
		/**
		 * Constructs a new Property.
		 *
		 * \param[in] value the value with which to initialize the Property.
		 */
		Property(const T &value) : value(value) {
		}

		/**
		 * Destroys the Property.
		 */
		~Property() {
		}

		/**
		 * Returns the signal fired when the value of the Property is about to change.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changing() const {
			return signal_changing_;
		}

		/**
		 * Returns the signal fired when the value of the Property changes.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changed() const {
			return signal_changed_;
		}

		/**
		 * Assigns a new value to the Property.
		 *
		 * \param[in] val the new value to assign.
		 */
		Property &operator=(const T &val) {
			if (value != val) {
				signal_changing().emit();
				value = val;
				signal_changed().emit();
			}
			return *this;
		}

		/**
		 * Assigns a new value to the Property.
		 *
		 * \param[in] val the new value to assign.
		 */
		Property &operator=(const Property<T> &val) {
			if (value != val.value) {
				signal_changing().emit();
				value = val.value;
				signal_changed().emit();
			}
			return *this;
		}

		/**
		 * Returns the value of the Property.
		 *
		 * \return the value.
		 */
		operator T() const {
			return value;
		}

		/**
		 * Returns the value of the Property.
		 *
		 * \return the value.
		 */
		const T &operator->() const {
			return value;
		}

	private:
		T value;
		mutable sigc::signal<void> signal_changing_;
		mutable sigc::signal<void> signal_changed_;
};

#endif

