#ifndef ROBOT_CONTROLLER_JOYSTICK_JOYSTICK_H
#define ROBOT_CONTROLLER_JOYSTICK_JOYSTICK_H

#include "util/fd.h"
#include "util/byref.h"
#include <vector>
#include <utility>
#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// Allows access to a joystick device.
//
class joystick : public virtual byref, public virtual sigc::trackable {
	public:
		//
		// A pointer to a joystick.
		//
		typedef Glib::RefPtr<joystick> ptr;

		//
		// Opens a joystick.
		//
		static ptr create(const Glib::ustring &filename) {
			ptr p(new joystick(filename));
			return p;
		}

		//
		// Returns the number of axes.
		//
		unsigned int axes() const { return axes_data.size(); }

		//
		// Returns the position of an axis in the range ±32767.
		//
		int axis(unsigned int i) const { return axes_data[i]; }

		//
		// Returns the number of buttons.
		//
		unsigned int buttons() const { return buttons_data.size(); }

		//
		// Returns the position of a button.
		//
		bool button(unsigned int i) const { return buttons_data[i]; }

		//
		// Returns the filename of the stick.
		//
		const Glib::ustring &filename() const { return stick_filename; }

		//
		// Returns the model name of the stick.
		//
		const Glib::ustring &name() const { return stick_name; }

		//
		// Fired when the joystick changes state.
		//
		sigc::signal<void> &signal_moved() { return sig_moved; }

		//
		// Returns a list of all the joysticks on the system.
		// The first element of the pair is the filename.
		// The second element of the pair is the model name.
		//
		static const std::vector<std::pair<Glib::ustring, Glib::ustring> > &list();

	private:
		file_descriptor fd;
		std::vector<int> axes_data;
		std::vector<bool> buttons_data;
		Glib::ustring stick_filename;
		Glib::ustring stick_name;
		sigc::signal<void> sig_moved;

		joystick(const Glib::ustring &filename);
		bool on_readable(Glib::IOCondition);
};

#endif

