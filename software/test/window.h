#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "test/chicker.h"
#include "test/drive.h"
#include "test/feedback.h"
#include "test/params.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * \brief A window that allows the user to control one robot for testing.
 */
class TesterWindow : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new TesterWindow.
		 *
		 * \param[in] dongle the radio dongle.
		 *
		 * \param[in] robot the robot to talk to.
		 */
		TesterWindow(XBeeDongle &dongle, XBeeRobot::Ptr robot);

	private:
		XBeeRobot::Ptr robot;

		Gtk::VBox vbox;

		Gtk::Frame feedback_frame;
		TesterFeedbackPanel feedback_panel;

		Gtk::Frame drive_frame;
		DrivePanel drive_panel;

		Gtk::ToggleButton dribble_button;

		Gtk::Frame chicker_frame;
		ChickerPanel chicker_panel;

		Gtk::Frame params_frame;
		TesterParamsPanel params_panel;

		int key_snoop(Widget *, GdkEventKey *event);
		void on_dribble_toggled();
};

#endif

