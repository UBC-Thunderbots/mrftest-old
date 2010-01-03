#include "firmware/emergency_erase.h"
#include "firmware/upload.h"
#include "firmware/window.h"
#include "uicomponents/bot_chooser.h"
#include "util/ihex.h"
#include "util/xml.h"
#include "world/config.h"
#include <iomanip>

namespace {
	class working_dialog : public Gtk::Dialog {
		public:
			working_dialog(Gtk::Window &win, watchable_operation &op) : Gtk::Dialog("Progress", win, true), op(op) {
				op.signal_error().connect(sigc::mem_fun(*this, &working_dialog::error));
				op.signal_progress().connect(sigc::mem_fun(*this, &working_dialog::status_update));
				op.signal_finished().connect(sigc::bind(sigc::mem_fun(static_cast<Gtk::Dialog &>(*this), &Gtk::Dialog::response), Gtk::RESPONSE_ACCEPT));
				pb.set_text(op.get_status());
				get_vbox()->pack_start(pb, false, false);
				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				show_all();
			}

		private:
			watchable_operation &op;
			Gtk::ProgressBar pb;

			void status_update(double fraction) {
				pb.set_fraction(fraction);
				pb.set_text(op.get_status());
			}

			void error(const Glib::ustring &message) {
				Gtk::MessageDialog md(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md.run();
				response(Gtk::RESPONSE_CANCEL);
			}
	};
}

class firmware_window_impl : public Gtk::Window {
	public:
		firmware_window_impl(xbee &modem, xmlpp::Element *xmlworld) : modem(modem), bot_frame("Bot"), bot_controls(xmlworld, *this), file_frame("Firmware File"), start_upload_button(Gtk::Stock::EXECUTE), emergency_erase_button("Emergency Erase") {
			set_title("Thunderbots");

			bot_controls.signal_address_changed().connect(sigc::mem_fun(*this, &firmware_window_impl::address_changed));
			bot_frame.add(bot_controls);
			vbox.pack_start(bot_frame, false, false);

			Gtk::FileFilter *filter = new Gtk::FileFilter();
			filter->set_name("Intel HEX Files");
			filter->add_pattern("*.mcs");
			file_chooser.add_filter(*filter);
			filter = new Gtk::FileFilter();
			filter->set_name("All Files");
			filter->add_pattern("*");
			file_chooser.add_filter(*filter);
			file_frame.add(file_chooser);
			vbox.pack_start(file_frame, false, false);

			start_upload_button.signal_clicked().connect(sigc::mem_fun(*this, &firmware_window_impl::start_upload));
			start_upload_button.set_sensitive(false);
			vbox.pack_start(start_upload_button, false, false);

			emergency_erase_button.signal_clicked().connect(sigc::mem_fun(*this, &firmware_window_impl::start_emergency_erase));
			vbox.pack_start(emergency_erase_button, false, false);

			add(vbox);

			show_all();
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		xbee &modem;

		uint64_t current_address;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		bot_chooser bot_controls;

		Gtk::Frame file_frame;
		Gtk::FileChooserButton file_chooser;

		Gtk::Button start_upload_button;
		Gtk::Button emergency_erase_button;

		void address_changed(uint64_t address) {
			current_address = address;
			start_upload_button.set_sensitive(!!address);
		}

		void start_upload() {
			const Glib::ustring &filename = file_chooser.get_filename();
			intel_hex ihex;
			try {
				ihex.load(filename);
			} catch (const std::runtime_error &exp) {
				Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md.run();
				return;
			}

			Glib::Timer timer;
			unsigned int crc_errors;
			{
				upload up(modem, current_address, ihex);
				working_dialog dlg(*this, up);
				Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(up, &upload::start), false));
				dlg.run();
				crc_errors = up.crc_failure_count();
			}
			timer.stop();

			const Glib::ustring &msg = Glib::ustring::compose(
					"Upload completed in %1s.\n%2 CRC errors repaired.",
					Glib::ustring::format(std::fixed, std::setprecision(1), timer.elapsed()),
					crc_errors);
			Gtk::MessageDialog md(*this, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
			md.run();
		}

		void start_emergency_erase() {
			int resp;
			{
				emergency_erase ee(modem, current_address);
				working_dialog dlg(*this, ee);
				Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(ee, &emergency_erase::start), false));
				resp = dlg.run();
			}
			if (resp == Gtk::RESPONSE_ACCEPT) {
				Gtk::MessageDialog md(*this, "The emergency erase was requested. The indicator LED should be blinking fast; when it occults slowly, power cycle the logic board.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
				md.run();
			}
		}
};

firmware_window::firmware_window(xbee &modem, xmlpp::Element *xmlworld) : impl(new firmware_window_impl(modem, xmlworld)) {
}

firmware_window::~firmware_window() {
	delete impl;
}

