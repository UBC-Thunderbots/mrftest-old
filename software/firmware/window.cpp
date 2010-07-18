#include "firmware/claim.h"
#include "firmware/emergency_erase.h"
#include "firmware/fpga.h"
#include "firmware/pic.h"
#include "firmware/watchable_pair.h"
#include "firmware/window.h"
#include "uicomponents/single_bot_combobox.h"
#include "util/config.h"
#include "util/ihex.h"
#include "xbee/client/raw.h"
#include <gtkmm.h>
#include <iomanip>

namespace {
	class WorkingDialog : public Gtk::Dialog {
		public:
			WorkingDialog(Gtk::Window &win, WatchableOperation &op) : Gtk::Dialog("Progress", win, true), op(op) {
				op.signal_error.connect(sigc::mem_fun(this, &WorkingDialog::error));
				op.signal_progress.connect(sigc::mem_fun(this, &WorkingDialog::status_update));
				op.signal_finished.connect(sigc::bind(sigc::mem_fun(static_cast<Gtk::Dialog *>(this), &Gtk::Dialog::response), Gtk::RESPONSE_ACCEPT));
				pb.set_text(op.get_status());
				get_vbox()->pack_start(pb, false, false);
				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				show_all();
			}

		private:
			WatchableOperation &op;
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

FirmwareWindow::FirmwareWindow(XBeeLowLevel &modem, const Config &conf, const Glib::ustring &robot, const std::string &filename) : modem(modem), bot_frame("Bot"), bot_controls(conf.robots(), robot), file_frame("Firmware File"), file_fpga_button(file_target_group, "FPGA"), file_pic_button(file_target_group, "PIC"), start_upload_button(Gtk::Stock::EXECUTE), emergency_erase_button("Emergency Erase") {
	set_title("Firmware Uploader");

	bot_frame.add(bot_controls);
	vbox.pack_start(bot_frame, false, false);

	Gtk::FileFilter *filter = new Gtk::FileFilter();
	filter->set_name("Intel HEX Files");
	filter->add_pattern("*.hex");
	filter->add_pattern("*.mcs");
	file_chooser.add_filter(*filter);
	filter = new Gtk::FileFilter();
	filter->set_name("All Files");
	filter->add_pattern("*");
	file_chooser.add_filter(*filter);
	if (!filename.empty()) {
		file_chooser.set_filename(Glib::filename_to_utf8(filename));
	}
	file_vbox.add(file_chooser);
	file_target_hbox.pack_start(file_fpga_button);
	file_target_hbox.pack_start(file_pic_button);
	file_vbox.pack_start(file_target_hbox);
	file_frame.add(file_vbox);
	vbox.pack_start(file_frame, false, false);

	start_upload_button.signal_clicked().connect(sigc::mem_fun(this, &FirmwareWindow::start_upload));
	vbox.pack_start(start_upload_button, false, false);

	emergency_erase_button.signal_clicked().connect(sigc::mem_fun(this, &FirmwareWindow::start_emergency_erase));
	vbox.pack_start(emergency_erase_button, false, false);

	add(vbox);

	show_all();
}

bool FirmwareWindow::on_delete_event(GdkEventAny *) {
	Gtk::Main::quit();
	return true;
}

void FirmwareWindow::start_upload() {
	const Glib::ustring &filename = file_chooser.get_filename();
	const uint64_t address = bot_controls.address();
	if (!address) {
		return;
	}
	const XBeeRawBot::ptr bot(XBeeRawBot::create(address, modem));
	Claim cl(bot);

	Glib::Timer timer;
	if (file_fpga_button.get_active()) {
		IntelHex ihex;
		ihex.add_section(0, 16 * 1024 * 1024 / 8);
		try {
			ihex.load(filename);
		} catch (const std::runtime_error &exp) {
			Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
			return;
		}
		FPGAUpload up(bot, ihex);
		WatchablePair p(cl, up, 0.01);
		WorkingDialog dlg(*this, p);
		Glib::signal_idle().connect_once(sigc::mem_fun(p, &WatchablePair::start));
		dlg.run();
	} else {
		IntelHex ihex;
		ihex.add_section(0x0, 0x800);
		ihex.add_section(0x800, 0x3FFF - 0x800 + 1);
		ihex.add_section(0x300000, 16);
		try {
			ihex.load(filename);
		} catch (const std::runtime_error &exp) {
			Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
			return;
		}
		PICUpload up(bot, ihex);
		WatchablePair p(cl, up, 0.01);
		WorkingDialog dlg(*this, p);
		Glib::signal_idle().connect_once(sigc::mem_fun(p, &WatchablePair::start));
		dlg.run();
	}
	timer.stop();

	const Glib::ustring &msg = Glib::ustring::compose("Upload completed in %1s.", Glib::ustring::format(std::fixed, std::setprecision(1), timer.elapsed()));
	Gtk::MessageDialog md(*this, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
	md.run();
}

void FirmwareWindow::start_emergency_erase() {
	int resp;
	{
		const uint64_t address = bot_controls.address();
		if (!address) {
			return;
		}
		const XBeeRawBot::ptr bot(XBeeRawBot::create(address, modem));
		Claim cl(bot);
		EmergencyErase ee(bot);
		WatchablePair p(cl, ee, 0.5);
		WorkingDialog dlg(*this, p);
		Glib::signal_idle().connect_once(sigc::mem_fun(p, &WatchablePair::start), false);
		resp = dlg.run();
	}
	if (resp == Gtk::RESPONSE_ACCEPT) {
		Gtk::MessageDialog md(*this, "The emergency erase was requested. The indicator LED should be blinking fast; when it occults slowly, power cycle the logic board.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
		md.run();
	}
}

