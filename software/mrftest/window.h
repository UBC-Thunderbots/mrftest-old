#ifndef MRFTESTER_WINDOW_H
#define MRFTESTER_WINDOW_H

#include "mrftest/kicker.h"
#include "mrftest/drive.h"
#include "mrftest/feedback.h"
#include "mrf/dongle.h"
#include "mrf/robot.h"
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
		TesterWindow(MRFDongle &dongle, MRFRobot &robot);

		/**
		 * \brief Destroys a TesterWindow.
		 */
		~TesterWindow();

	private:
		class MappedJoysticksModel;

		Glib::RefPtr<MappedJoysticksModel> mapped_joysticks;

		MRFRobot &robot;

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

