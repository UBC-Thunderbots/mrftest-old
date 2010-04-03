#include "tester/controlled_permotor_drive.h"
#include "tester/direct_drive.h"
#include "tester/feedback.h"
#include "tester/matrix_drive.h"
#include "tester/window.h"
#include "uicomponents/bot_chooser.h"
#include "util/ihex.h"
#include "util/xml.h"
#include "world/config.h"
#include "xbee/bot.h"
#include <iomanip>

class tester_window_impl : public Gtk::Window {
	public:
		tester_window_impl(xbee &modem, xmlpp::Element *xmlworld) : modem(modem), bot_frame("Bot"), bot_controls(xmlworld, *this), feedback_frame("Feedback"), command_frame("Commands"), drive_frame("Drive"), drive_widget(0), dribble_frame("Dribble"), dribble_scale(-1023, 1023, 1), chicker_frame("Chicker"), chicker_enabled("Enable") {
			set_title("Robot Tester");

			bot_controls.signal_address_changed().connect(sigc::mem_fun(*this, &tester_window_impl::address_changed));
			bot_frame.add(bot_controls);
			vbox.pack_start(bot_frame, false, false);

			feedback_frame.add(feedback);
			vbox.pack_start(feedback_frame, false, false);

			vbox.pack_start(command_frame, false, false);

			drive_chooser.append_text("Halt");
			drive_chooser.append_text("Direct Drive");
			drive_chooser.append_text("Controlled Per-Motor Drive");
			drive_chooser.append_text("Matrix Drive");
			drive_chooser.set_active_text("Halt");
			drive_chooser.signal_changed().connect(sigc::mem_fun(*this, &tester_window_impl::drive_mode_changed));
			drive_box.pack_start(drive_chooser);
			drive_frame.add(drive_box);
			vbox.pack_start(drive_frame, false, false);

			dribble_scale.get_adjustment()->set_page_size(0);
			dribble_box.pack_start(dribble_scale);
			dribble_frame.add(dribble_box);
			vbox.pack_start(dribble_frame, false, false);

			chicker_box.pack_start(chicker_enabled, true, true);
			chicker_box.pack_start(chicker_status, false, false);
			chicker_frame.add(chicker_box);
			vbox.pack_start(chicker_frame, false, false);

			add(vbox);

			show_all();

			Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(*this, &tester_window_impl::key_snoop));
			dribble_scale.set_value(0);
			dribble_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_window_impl::on_dribble_change));
			chicker_enabled.signal_toggled().connect(sigc::mem_fun(*this, &tester_window_impl::on_chicker_enable_change));

			on_update();
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		xbee &modem;
		radio_bot::ptr bot;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		bot_chooser bot_controls;

		Gtk::Frame feedback_frame;
		tester_feedback feedback;

		Gtk::Frame command_frame;

		Gtk::Frame drive_frame;
		Gtk::VBox drive_box;
		Gtk::ComboBoxText drive_chooser;
		tester_control_direct_drive drive_direct;
		tester_control_controlled_permotor_drive drive_controlled_permotor;
		tester_control_matrix_drive drive_matrix;
		Gtk::Widget *drive_widget;

		Gtk::Frame dribble_frame;
		Gtk::VBox dribble_box;
		Gtk::HScale dribble_scale;

		Gtk::Frame chicker_frame;
		Gtk::HBox chicker_box;
		Gtk::CheckButton chicker_enabled;
		light chicker_status;

		void address_changed(uint64_t address) {
			// Update the bot pointer.
			if (address) {
				bot = radio_bot::ptr(new radio_bot(modem, address));
				bot->start();
			} else {
				bot.reset();
			}

			// Attach the robot to the feedback controls, dropping any prior.
			feedback.set_bot(bot);

			// Attach the robot to the drive controls, dropping any prior.
			attach_drive_controls_to_bot();

			// Attach the robot to the dribbler controls, dropping any prior.
			on_dribble_change();

			// Attach to the update handler.
			if (bot) {
				bot->signal_updated().connect(sigc::mem_fun(*this, &tester_window_impl::on_update));
			}
		}

		int key_snoop(Widget *, GdkEventKey *event) {
			if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
				// Z letter scrams the system.
				drive_chooser.set_active_text("Halt");
				dribble_scale.set_value(0);
			} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_0)) {
				// Zero digit sets all controls to zero but does not scram things.
				drive_direct.zero();
				drive_controlled_permotor.zero();
				drive_matrix.zero();
				dribble_scale.set_value(0);
			}
			return 0;
		}

		void drive_mode_changed() {
			// Remove any UI control.
			if (drive_widget) {
				drive_box.remove(*drive_widget);
				drive_widget = 0;
			}

			// Zero out all drive controls.
			drive_direct.zero();
			drive_controlled_permotor.zero();
			drive_matrix.zero();

			// Detach the bot from all drive controls.
			drive_direct.set_robot(radio_bot::ptr());
			drive_controlled_permotor.set_robot(radio_bot::ptr());
			drive_matrix.set_robot(radio_bot::ptr());

			// Attach the new controls to the robot.
			attach_drive_controls_to_bot();

			// Place the new controls in the UI.
			if (drive_widget) {
				drive_box.pack_start(*drive_widget);
				drive_widget->show_all();
			}
		}

		void attach_drive_controls_to_bot() {
			const Glib::ustring &cur = drive_chooser.get_active_text();
			if (cur == "Halt") {
				// No controls, but need to scram the drive motors.
				drive_widget = 0;
				if (bot) {
					bot->drive_scram();
				}
			} else if (cur == "Direct Drive") {
				// Use the direct drive controls.
				drive_widget = &drive_direct;
				drive_direct.set_robot(bot);
			} else if (cur == "Controlled Per-Motor Drive") {
				// Use the controlled per-motor drive controls.
				drive_widget = &drive_controlled_permotor;
				drive_controlled_permotor.set_robot(bot);
			} else if (cur == "Matrix Drive") {
				// Use the matrix drive controls.
				drive_widget = &drive_matrix;
				drive_matrix.set_robot(bot);
			}
		}

		void on_dribble_change() {
			if (bot) {
				bot->dribble(dribble_scale.get_value());
			}
		}

		void on_chicker_enable_change() {
			if (bot) {
				bot->enable_chicker(chicker_enabled.get_active());
			}
		}

		void on_update() {
			if (bot && bot->has_feedback()) {
				if (bot->chicker_faulted()) {
					chicker_status.set_colour(1, 0, 0);
				} else if (bot->chicker_ready()) {
					chicker_status.set_colour(0, 1, 0);
				} else {
					chicker_status.set_colour(0, 0, 0);
				}
			} else {
				chicker_status.set_colour(0, 0, 0);
			}
		}
};

tester_window::tester_window(xbee &modem, xmlpp::Element *xmlworld) : impl(new tester_window_impl(modem, xmlworld)) {
}

tester_window::~tester_window() {
	delete impl;
}

