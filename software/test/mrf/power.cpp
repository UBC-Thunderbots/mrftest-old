#include "test/mrf/power.h"

PowerPanel::PowerPanel(MRFDongle &dongle, unsigned int index) : Gtk::Table(3, 2), dongle(dongle), index(index), drivetrain_button(u8"Power Drivetrain"), kicker_button(u8"Power Kicker"), relay_button(u8"Chicker Emergency Relay"), current_operation_status_entry(0) {
	drivetrain_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::power_drivetrain));
	attach(drivetrain_button, 0, 1, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	drivetrain_status.set_editable(false);
	attach(drivetrain_status, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	kicker_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::power_kicker));
	attach(kicker_button, 0, 1, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	kicker_status.set_editable(false);
	attach(kicker_status, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	relay_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::power_relay));
	attach(relay_button, 0, 1, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	relay_status.set_editable(false);
	attach(relay_status, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

void PowerPanel::power_drivetrain() {
	current_operation_status_entry = &drivetrain_status;
	send_message(0x09);
}

void PowerPanel::power_kicker() {
	current_operation_status_entry = &kicker_status;
	send_message(0x0A);
}

void PowerPanel::power_relay() {
	current_operation_status_entry = &relay_status;
	send_message(0x0D);
}

void PowerPanel::send_message(uint8_t code) {
	drivetrain_button.set_sensitive(false);
	kicker_button.set_sensitive(false);

	current_operation_status_entry->set_text(u8"Sending…");

	message.reset(new MRFDongle::SendReliableMessageOperation(dongle, index, &code, sizeof(code)));
	message->signal_done.connect(sigc::mem_fun(this, &PowerPanel::check_result));
}

void PowerPanel::check_result(AsyncOperation<void> &op) {
	try {
		op.result();
		current_operation_status_entry->set_text(u8"OK");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		current_operation_status_entry->set_text(u8"Not associated");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		current_operation_status_entry->set_text(u8"Not acknowledged");
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		current_operation_status_entry->set_text(u8"CCA fail");
	}

	message.reset();
	current_operation_status_entry = 0;

	drivetrain_button.set_sensitive();
	kicker_button.set_sensitive();
}

