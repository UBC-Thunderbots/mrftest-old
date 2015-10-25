#ifndef TEST_NULL_WINDOW_H
#define TEST_NULL_WINDOW_H

#include "drive/null/dongle.h"
#include "drive/null/robot.h"
#include "test/common/dribble.h"
#include "test/common/drive.h"
#include "test/common/feedback.h"
#include "test/common/kicker.h"
#include "test/mrf/leds.h"
#include "test/mrf/power.h"
#include <vector>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/frame.h>
#include <gtkmm/window.h>

/**
 * \brief A window that allows the user to control one robot for testing.
 */
class NullTesterWindow final : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new TesterWindow.
		 *
		 * \param[in] dongle the radio dongle.
		 *
		 * \param[in] robot the robot to talk to.
		 */
		explicit NullTesterWindow(Drive::Dongle &dongle, Drive::Robot &robot);

		/**
		 * \brief Destroys a TesterWindow.
		 */
		~NullTesterWindow();

	private:
		class MappedJoysticksModel;

		Glib::RefPtr<MappedJoysticksModel> mapped_joysticks;

		Gtk::VBox outer_vbox;

		Gtk::HBox hbox;

		Gtk::VBox vbox1;

		Gtk::Frame feedback_frame;
		TesterFeedbackPanel feedback_panel;

		Gtk::Frame leds_frame;
		Gtk::Label leds_label;

		Gtk::Frame power_frame;
		Gtk::Label power_label;

		Gtk::VBox vbox2;

		Gtk::Frame drive_frame;
		DrivePanel drive_panel;

		Gtk::Frame kicker_frame;
		KickerPanel kicker_panel;

		Gtk::Frame dribble_frame;
		DribblePanel dribble_panel;

		Gtk::Frame joystick_frame;
		Gtk::HBox joystick_sensitivity_hbox;
		Gtk::RadioButtonGroup joystick_sensitivity_group;
		Gtk::RadioButton joystick_sensitivity_high_button, joystick_sensitivity_low_button;
		Gtk::VBox joystick_vbox;
		Gtk::ComboBox joystick_chooser;

		std::vector<sigc::connection> joystick_signal_connections;

		Drive::Robot *robot;

		void scram();
		int key_snoop(Widget *, GdkEventKey *event);
		void on_joystick_chooser_changed();
		void on_joystick_drive_axis_changed();
		void on_joystick_dribble_changed();
		void on_joystick_kick_changed();
		void on_joystick_scram_changed();
		bool on_delete_event(GdkEventAny *);
};

#endif
