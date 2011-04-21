#ifndef UTIL_JOYSTICK_H
#define UTIL_JOYSTICK_H

#include "util/byref.h"
#include "util/fd.h"
#include "util/property.h"
#include <glibmm.h>
#include <string>
#include <vector>
#include <sigc++/sigc++.h>

/**
 * \brief Provides access to a joystick attached to the computer.
 */
class Joystick : public ByRef, public sigc::trackable {
	private:
		const FileDescriptor::Ptr fd;

	public:
		/**
		 * \brief A pointer to a Joystick.
		 */
		typedef RefPtr<Joystick> Ptr;

		/**
		 * \brief The device node of the joystick.
		 */
		const std::string node;

		/**
		 * \brief The manufacturer and model of the joystick.
		 */
		const Glib::ustring name;

		/**
		 * \brief Returns a vector of all the joysticks attached to the computer.
		 *
		 * On first invocation, this will scan for joysticks.
		 * Subsequent invocations will return the original data.
		 *
		 * \return the joysticks.
		 */
		static const std::vector<Ptr> &all();

		/**
		 * \brief Returns the joystick's axes.
		 *
		 * \return the axes.
		 */
		const std::vector<Property<double> > &axes() const;

		/**
		 * \brief Returns the joystick's buttons.
		 *
		 * \return the buttons.
		 */
		const std::vector<Property<bool> > &buttons() const;

	private:
		std::vector<Property<double> > axes_;
		std::vector<Property<bool> > buttons_;

		Joystick(const std::string &node);
		void on_readable();
};

inline const std::vector<Property<double> > &Joystick::axes() const {
	return axes_;
}

inline const std::vector<Property<bool> > &Joystick::buttons() const {
	return buttons_;
}

#endif

