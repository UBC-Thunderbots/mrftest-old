#include "mrftest/params.h"
#include "util/algorithm.h"
#include "util/string.h"
#include <cstddef>
#include <iomanip>
#include <locale>
#include <sstream>
#include <gtkmm/messagedialog.h>

namespace {
	Glib::ustring format_channel(unsigned int ch) {
		return Glib::ustring::compose("%1 (%2)", tohex(ch, 2), ch);
	}
}

ParamsPanel::ParamsPanel(MRFDongle &dongle, MRFRobot &robot) : Gtk::Table(4, 2), dongle(dongle), robot(robot), channel_label(u8"Channel:"), index_label(u8"Index:"), pan_label(u8"PAN (hex):"), set(u8"Set"), reboot(u8"Reboot") {
	for (unsigned int ch = 0x0B; ch <= 0x1A; ++ch) {
		channel_chooser.append_text(format_channel(ch));
	}
	for (unsigned int i = 0; i <= 7; ++i) {
		index_chooser.append_text(Glib::ustring::format(i));
	}
	pan_entry.set_width_chars(4);
	pan_entry.set_max_length(4);
	set.signal_clicked().connect(sigc::mem_fun(this, &ParamsPanel::send_params));
	reboot.signal_clicked().connect(sigc::mem_fun(this, &ParamsPanel::reboot_robot));

	attach(channel_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channel_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index_chooser, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pan_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pan_entry, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	hbb.pack_start(set);
	hbb.pack_start(reboot);
	attach(hbb, 0, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

void ParamsPanel::activate_controls(bool act) {
	channel_chooser.set_sensitive(act);
	index_chooser.set_sensitive(act);
	pan_entry.set_sensitive(act);
	set.set_sensitive(act);
	reboot.set_sensitive(act);
}

void ParamsPanel::send_params() {
	if (channel_chooser.get_active_row_number() >= 0 && index_chooser.get_active_row_number() >= 0) {
		uint8_t channel = static_cast<uint8_t>(channel_chooser.get_active_row_number() + 0x0B);
		uint8_t index = static_cast<uint8_t>(index_chooser.get_active_row_number());
		std::wistringstream iss(ustring2wstring(pan_entry.get_text()));
		iss.flags(std::ios_base::hex);
		uint16_t pan;
		iss >> pan;
		if (!iss || pan == 0xFFFF) {
			Gtk::MessageDialog md(u8"Invalid PAN ID", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
			return;
		}
		uint8_t packet[5];
		packet[0] = 0x0B;
		packet[1] = channel;
		packet[2] = index;
		packet[3] = static_cast<uint8_t>(pan);
		packet[4] = static_cast<uint8_t>(pan >> 8);
		set.set_label(u8"Sending…");
		activate_controls(false);
		message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, packet, sizeof(packet)));
		message->signal_done.connect(sigc::mem_fun(this, &ParamsPanel::check_result));
		rebooting = false;
	}
}

void ParamsPanel::reboot_robot() {
	if (channel_chooser.get_active_row_number() >= 0 && index_chooser.get_active_row_number() >= 0) {
		uint8_t packet[1];
		packet[0] = 0x08;
		set.set_label(u8"Sending…");
		activate_controls(false);
		message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, packet, sizeof(packet)));
		message->signal_done.connect(sigc::mem_fun(this, &ParamsPanel::check_result));
		rebooting = true;
	}
}

void ParamsPanel::check_result(AsyncOperation<void> &op) {
	try {
		op.result();
		(rebooting ? reboot : set).set_label(u8"OK");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		(rebooting ? reboot : set).set_label(u8"Not associated");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		(rebooting ? reboot : set).set_label(u8"Not acknowledged");
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		(rebooting ? reboot : set).set_label(u8"CCA fail");
	}
	message.reset();
	activate_controls(true);
}

