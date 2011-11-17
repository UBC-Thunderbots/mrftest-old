#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "test/kicker.h"
#include "test/drive.h"
#include "test/feedback.h"
#include "test/params.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <vector>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/frame.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

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

		/**
		 * \brief Destroys a TesterWindow.
		 */
		~TesterWindow();

	private:
		class MappedJoysticksModel;

		Glib::RefPtr<MappedJoysticksModel> mapped_joysticks;

		XBeeRobot::Ptr robot;

		Gtk::VBox outer_vbox;

		Gtk::HBox hbox;

		Gtk::VBox vbox1;

		Gtk::Frame feedback_frame;
		TesterFeedbackPanel feedback_panel;

		Gtk::Frame drive_frame;
		DrivePanel drive_panel;

		Gtk::ToggleButton dribble_button;

		Gtk::VBox vbox2;

		Gtk::Frame kicker_frame;
		KickerPanel kicker_panel;

		Gtk::Frame params_frame;
		TesterParamsPanel params_panel;

		Gtk::Frame joystick_frame;
		Gtk::HBox joystick_sensitivity_hbox;
		Gtk::RadioButtonGroup joystick_sensitivity_group;
		Gtk::RadioButton joystick_sensitivity_high_button, joystick_sensitivity_low_button;
		Gtk::VBox joystick_vbox;
		Gtk::ComboBox joystick_chooser;

		std::vector<sigc::connection> joystick_signal_connections;

		void scram();
		int key_snoop(Widget *, GdkEventKey *event);
		void on_dribble_toggled();
		void on_joystick_chooser_changed();
		void on_joystick_drive_axis_changed();
		void on_joystick_dribble_changed();
		void on_joystick_kick_changed();
		void on_joystick_scram_changed();
		bool on_delete_event(GdkEventAny *);
};

#endif

