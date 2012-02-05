#ifndef UTIL_JOYSTICK_H
#define UTIL_JOYSTICK_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <glibmm/ustring.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

/**
 * \brief Provides access to a joystick attached to the computer.
 */
class Joystick : public NonCopyable, public sigc::trackable {
	public:
		/**
		 * \brief Returns the number of joysticks attached to the computer.
		 *
		 * \return the number of joysticks.
		 */
		static std::size_t count();

		/**
		 * \brief Returns a joystick by index.
		 *
		 * \pre 0 ≤ \p index < count()
		 *
		 * \param[in] index the zero-based index of the joystick to retrieve.
		 *
		 * \return the joystick.
		 */
		static const Joystick &get(std::size_t index);

		/**
		 * \brief Returns the device node of the joystick.
		 *
		 * \return the device node of the joystick.
		 */
		const std::string &node() const;

		/**
		 * \brief Returns the manufacturer and model of the joystick.
		 *
		 * \return the manufacturer and model of the joystick.
		 */
		const Glib::ustring &name() const;

		/**
		 * \brief Emitted whenever any axis or button changes value.
		 */
		sigc::signal<void> &signal_changed() const;

		/**
		 * \brief Returns the joystick's axes.
		 *
		 * \return the axes.
		 */
		const std::vector<Property<double>> &axes() const;

		/**
		 * \brief Returns the joystick's buttons.
		 *
		 * \return the buttons.
		 */
		const std::vector<Property<bool>> &buttons() const;

	private:
		FileDescriptor fd;
		std::string node_;
		Glib::ustring name_;
		mutable sigc::signal<void> signal_changed_;
		std::vector<Property<double>> axes_;
		std::vector<Property<bool>> buttons_;

		static std::vector<std::unique_ptr<Joystick>> &instances();

		Joystick(const std::string &node);
		void on_readable();
};

bool operator<(const Joystick &x, const Joystick &y);



inline std::size_t Joystick::count() {
	return instances().size();
}

inline const Joystick &Joystick::get(std::size_t index) {
	return *instances()[index].get();
}

inline const std::string &Joystick::node() const {
	return node_;
}

inline const Glib::ustring &Joystick::name() const {
	return name_;
}

inline sigc::signal<void> &Joystick::signal_changed() const {
	return signal_changed_;
}

inline const std::vector<Property<double> > &Joystick::axes() const {
	return axes_;
}

inline const std::vector<Property<bool> > &Joystick::buttons() const {
	return buttons_;
}

#endif

