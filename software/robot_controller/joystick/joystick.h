#ifndef ROBOT_CONTROLLER_JOYSTICK_JOYSTICK_H
#define ROBOT_CONTROLLER_JOYSTICK_JOYSTICK_H

#include "util/byref.h"
#include "util/fd.h"
#include <glibmm.h>

/**
 * Allows access to a joystick device.
 */
class Joystick : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to a Joystick.
		 */
		typedef RefPtr<Joystick> Ptr;

		/**
		 * Opens a joystick.
		 *
		 * \param[in] filename the name of the \c /dev/input file exporting the
		 * joystick data.
		 *
		 * \return the new Joystick object.
		 */
		static Ptr create(const Glib::ustring &filename) {
			Ptr p(new Joystick(filename));
			return p;
		}

		/**
		 * Returns the number of axes.
		 *
		 * \return the number of axes.
		 */
		unsigned int axes() const { return axes_data.size(); }

		/**
		 * Returns the position of an axis in the range ±32767.
		 *
		 * \param[in] i the axis number, from 0 to <code>axes() - 1</code>.
		 *
		 * \return the position of the axis.
		 */
		int axis(unsigned int i) const { return axes_data[i]; }

		/**
		 * Returns the number of buttons.
		 *
		 * \return the number of buttons.
		 */
		unsigned int buttons() const { return buttons_data.size(); }

		/**
		 * Returns the position of a button.
		 *
		 * \param[in] i the button number, from 0 to <code>buttons() - 1</code>.
		 *
		 * \return \c true if button \p i is pressed, or \c false if not.
		 */
		bool button(unsigned int i) const { return buttons_data[i]; }

		/**
		 * Returns the filename of the stick.
		 *
		 * \return the filename.
		 */
		const Glib::ustring &filename() const { return stick_filename; }

		/**
		 * Returns the model name of the stick.
		 *
		 * \return the model name.
		 */
		const Glib::ustring &name() const { return stick_name; }

		/**
		 * Fired when the joystick changes state.
		 */
		sigc::signal<void> &signal_moved() { return sig_moved; }

		/**
		 * Returns a list of all the joysticks on the system.
		 * The first element of the pair is the filename.
		 * The second element of the pair is the model name.
		 *
		 * \return the detected joysticks.
		 */
		static const std::vector<std::pair<Glib::ustring, Glib::ustring> > &list();

	private:
		const FileDescriptor::Ptr fd;
		std::vector<int> axes_data;
		std::vector<bool> buttons_data;
		Glib::ustring stick_filename;
		Glib::ustring stick_name;
		sigc::signal<void> sig_moved;

		Joystick(const Glib::ustring &filename);
		bool on_readable(Glib::IOCondition);
};

#endif

