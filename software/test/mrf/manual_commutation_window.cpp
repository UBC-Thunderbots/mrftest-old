#include "test/mrf/manual_commutation_window.h"
#include <algorithm>

ManualCommutationWindow::ManualCommutationWindow(MRFDongle &dongle, unsigned int index) : dongle(dongle), index(index) {
	set_title(Glib::ustring::compose(u8"Manual Commutation (%1)", index));

	std::fill_n(old_data, sizeof(old_data) / sizeof(*old_data), 0);

	for (unsigned int motor = 0; motor < 5; ++motor) {
		motor_frames[motor].set_label(Glib::ustring::compose(u8"Motor %1", motor));
		motor_tables[motor].resize(4, 3);

		for (unsigned int phase = 0; phase < 3; ++phase) {
			phase_buttons[motor][phase][0].set_label(u8"Float");
			phase_buttons[motor][phase][1].set_label(u8"PWM");
			phase_buttons[motor][phase][2].set_label(u8"Low");
			phase_buttons[motor][phase][3].set_label(u8"High");
			for (unsigned int mode = 0; mode < 4; ++mode) {
				phase_buttons[motor][phase][mode].set_group(phase_button_groups[motor][phase]);
				phase_buttons[motor][phase][mode].signal_toggled().connect(sigc::mem_fun(this, &ManualCommutationWindow::send_packet));
				motor_tables[motor].attach(phase_buttons[motor][phase][mode], phase, phase + 1, mode, mode + 1);
			}
		}

		motor_frames[motor].add(motor_tables[motor]);
		vbox.pack_start(motor_frames[motor]);
	}

	status_entry.set_sensitive(false);
	vbox.pack_start(status_entry);

	add(vbox);
}

void ManualCommutationWindow::send_packet() {
	uint8_t buffer[6] = { 0x0D, 0, 0, 0, 0, 0 };
	for (unsigned int motor = 0; motor < 5; ++motor) {
		for (unsigned int phase = 0; phase < 3; ++phase) {
			for (unsigned int mode = 0; mode < 4; ++mode) {
				if (phase_buttons[motor][phase][mode].get_active()) {
					buffer[motor + 1] |= static_cast<uint8_t>(mode << (phase * 2 + 2));
				}
			}
		}
	}
	if (!std::equal(buffer + 1, buffer + 1 + sizeof(old_data) / sizeof(*old_data), old_data)) {
		std::copy_n(buffer + 1, sizeof(old_data) / sizeof(*old_data), old_data);
		status_entry.set_text(u8"Sending…");
		set_controls_sensitive(false);
		message.reset(new MRFDongle::SendReliableMessageOperation(dongle, index, buffer, sizeof(buffer)));
		message->signal_done.connect(sigc::mem_fun(this, &ManualCommutationWindow::check_result));
	}
}

void ManualCommutationWindow::check_result(AsyncOperation<void> &op) {
	try {
		op.result();
		status_entry.set_text(u8"OK");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		status_entry.set_text(u8"Not associated");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		status_entry.set_text(u8"Not acknowledged");
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		status_entry.set_text(u8"CCA fail");
	}

	set_controls_sensitive(true);

	message.reset();
}

void ManualCommutationWindow::set_controls_sensitive(bool sensitive) {
	for (unsigned int motor = 0; motor < 5; ++motor) {
		for (unsigned int phase = 0; phase < 3; ++phase) {
			for (unsigned int mode = 0; mode < 4; ++mode) {
				phase_buttons[motor][phase][mode].set_sensitive(sensitive);
			}
		}
	}
}

