#include "tester/controls.h"
#include "tester/direct_drive.h"
#include "tester/feedback.h"
#include "tester/window.h"
#include "uicomponents/bot_chooser.h"
#include "util/ihex.h"
#include "util/xml.h"
#include "world/config.h"
#include "xbee/packettypes.h"
#include "xbee/util.h"
#include <iomanip>

class tester_window_impl : public Gtk::Window {
	public:
		tester_window_impl(xbee &modem, xmlpp::Element *xmlworld) : modem(modem), current_address(0), feedback_counter(0), bot_frame("Bot"), bot_controls(xmlworld, *this), feedback_frame("Feedback"), feedback(modem), command_frame("Commands"), control_frame("Controls"), current_controls(0), current_controls_widget(0) {
			set_title("Thunderbots");

			bot_controls.signal_address_changed().connect(sigc::mem_fun(*this, &tester_window_impl::address_changed));
			bot_frame.add(bot_controls);
			vbox.pack_start(bot_frame, false, false);

			feedback_frame.add(feedback);
			vbox.pack_start(feedback_frame, false, false);

			vbox.pack_start(command_frame, false, false);

			control_chooser.append_text("Halt");
			control_chooser.append_text("Direct Drive");
			control_chooser.set_active_text("Halt");
			control_chooser.signal_changed().connect(sigc::mem_fun(*this, &tester_window_impl::controls_changed));
			control_box.pack_start(control_chooser);
			control_frame.add(control_box);
			vbox.pack_start(control_frame, false, false);

			add(vbox);

			show_all();

			Glib::signal_timeout().connect(sigc::mem_fun(*this, &tester_window_impl::tick), (1000 + 15) / 30);
			Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(*this, &tester_window_impl::key_snoop));
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		xbee &modem;
		uint64_t current_address;
		unsigned int feedback_counter;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		bot_chooser bot_controls;

		Gtk::Frame feedback_frame;
		tester_feedback feedback;

		Gtk::Frame command_frame;

		Gtk::Frame control_frame;
		Gtk::VBox control_box;
		Gtk::ComboBoxText control_chooser;
		tester_control_direct_drive direct_drive;

		tester_controls *current_controls;
		Gtk::Widget *current_controls_widget;

		void address_changed(uint64_t address) {
			current_address = address;
			feedback.address_changed(address);
		}

		bool tick() {
			if (current_address) {
				xbeepacket::RUN_DATA data;
				data.txhdr.apiid = xbeepacket::TRANSMIT_APIID;
				data.txhdr.frame = 0;
				xbeeutil::address_to_bytes(current_address, data.txhdr.address);
				data.txhdr.options = xbeepacket::TRANSMIT_OPTION_DISABLE_ACK;
				data.flags = xbeepacket::RUN_FLAG_RUNNING;
				data.command_seq = 0;
				data.command = xbeepacket::RUN_COMMAND_NOOP;
				if (current_controls) {
					current_controls->encode(data);
				}
				if (++feedback_counter == 30) {
					feedback_counter = 0;
					data.flags |= xbeepacket::RUN_FLAG_FEEDBACK;
				}
				modem.send(&data, sizeof(data));
			}
			return true;
		}

		int key_snoop(Widget *, GdkEventKey *event) {
			if (current_address) {
				if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
					control_chooser.set_active_text("Halt");
					scram();
				} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_0)) {
					scram();
				}
			}
			return 0;
		}

		void controls_changed() {
			scram();
			const Glib::ustring &cur = control_chooser.get_active_text();
			if (current_controls_widget) {
				control_box.remove(*current_controls_widget);
				current_controls = 0;
				current_controls_widget = 0;
			}
			if (cur == "Halt") {
				// Leave it NULL.
			} else if (cur == "Direct Drive") {
				current_controls = &direct_drive;
				current_controls_widget = &direct_drive;
			}
			if (current_controls_widget) {
				control_box.pack_start(*current_controls_widget);
				current_controls_widget->show_all();
			}
		}

		void scram() {
			direct_drive.scram();
		}
};

tester_window::tester_window(xbee &modem, xmlpp::Element *xmlworld) : impl(new tester_window_impl(modem, xmlworld)) {
}

tester_window::~tester_window() {
	delete impl;
}

