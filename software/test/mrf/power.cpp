#include "test/mrf/power.h"

PowerPanel::PowerPanel(MRFDongle &dongle, unsigned int index) : Gtk::Table(1, 2), dongle(dongle), index(index), drivetrain_button(u8"Power Drivetrain"), current_operation_status_entry(0) {
	drivetrain_button.signal_clicked().connect(sigc::mem_fun(this, &PowerPanel::power_drivetrain));
	attach(drivetrain_button, 0, 1, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	drivetrain_status.set_editable(false);
	attach(drivetrain_status, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

void PowerPanel::power_drivetrain() {
	current_operation_status_entry = &drivetrain_status;
	send_message(0x09);
}

void PowerPanel::send_message(uint8_t code) {
	drivetrain_button.set_sensitive(false);

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
	current_operation_status_entry = nullptr;

	drivetrain_button.set_sensitive();
}

