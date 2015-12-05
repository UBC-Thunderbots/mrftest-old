#include "test/mrf/power.h"
#include <glibmm/main.h>

PowerPanel::PowerPanel(MRFDongle &dongle, unsigned int index) :
		Gtk::VButtonBox(Gtk::BUTTONBOX_SPREAD),
		dongle(dongle),
		index(index),
		power_drivetrain_button(u8"Power Drivetrain"),
		reboot_button(u8"Reboot"),
		shut_down_button(u8"Shut Down"),
		current_action_button(nullptr)
{
	power_drivetrain_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::power_drivetrain));
	reboot_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::reboot));
	shut_down_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::shut_down));
	pack_end(power_drivetrain_button);
	pack_end(reboot_button);
	pack_end(shut_down_button);
}

void PowerPanel::power_drivetrain() {
	send_message(0x09, power_drivetrain_button);
}

void PowerPanel::reboot() {
	send_message(0x08, reboot_button);
}

void PowerPanel::shut_down() {
	send_message(0x0C, shut_down_button);
}

void PowerPanel::send_message(uint8_t code, Gtk::Button &button) {
	set_buttons_sensitive(false);
	current_action_button = &button;
	button.set_label(u8"Sending…");
	message.reset(new MRFDongle::SendReliableMessageOperation(dongle, index, 20, &code, sizeof(code)));
	message->signal_done.connect(sigc::mem_fun(this, &PowerPanel::check_result));
}

void PowerPanel::check_result(AsyncOperation<void> &op) {
	Glib::ustring message;
	try {
		op.result();
		message = u8"OK";
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		message = u8"Not associated";
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		message = u8"Not acknowledged";
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		message = u8"CCA fail";
	}

	current_action_button->set_label(message);
	current_action_button = nullptr;
	Glib::signal_timeout().connect_once(sigc::mem_fun(this, &PowerPanel::timeout), 2000);
}

void PowerPanel::timeout() {
	power_drivetrain_button.set_label(u8"Power Drivetrain");
	reboot_button.set_label(u8"Reboot");
	shut_down_button.set_label(u8"Shut Down");
	set_buttons_sensitive(true);
}

void PowerPanel::set_buttons_sensitive(bool sensitive) {
	power_drivetrain_button.set_sensitive(sensitive);
	reboot_button.set_sensitive(sensitive);
	shut_down_button.set_sensitive(sensitive);
}
