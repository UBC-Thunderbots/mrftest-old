#include "mrftest/leds.h"
#include <stdint.h>

namespace {
	struct LEDMode {
		const char *name;
		uint8_t value;
	};

	const LEDMode MODES[] = {
		{ u8"Normal operation", 0x20 },
		{ u8"Lamp test", 0x21 },
		{ u8"Wheel 0 Hall sensors", 0x00 },
		{ u8"Wheel 1 Hall sensors", 0x01 },
		{ u8"Wheel 2 Hall sensors", 0x02 },
		{ u8"Wheel 3 Hall sensors", 0x03 },
		{ u8"Dribbler Hall sensors", 0x04 },
		{ u8"Wheel 0 encoders", 0x05 },
		{ u8"Wheel 1 encoders", 0x06 },
		{ u8"Wheel 2 encoders", 0x07 },
		{ u8"Wheel 3 encoders", 0x08 },
	};
}

LEDsPanel::LEDsPanel(MRFDongle &dongle, unsigned int index) : dongle(dongle), index(index), set_button(u8"Set") {
	for (std::size_t i = 0; i < sizeof(MODES) / sizeof(*MODES); ++i) {
		mode_chooser.append_text(MODES[i].name);
	}
	mode_chooser.set_active(0);
	pack_start(mode_chooser, Gtk::PACK_SHRINK);

	set_button.signal_clicked().connect(sigc::mem_fun(this, &LEDsPanel::set_mode));
	pack_start(set_button, Gtk::PACK_SHRINK);

	status_entry.set_sensitive(false);
	status_entry.set_width_chars(20);
	pack_start(status_entry, Gtk::PACK_SHRINK);
}

void LEDsPanel::set_mode() {
	if (mode_chooser.get_active() >= 0) {
		status_entry.set_text(u8"Sending…");
		set_button.set_sensitive(false);
		uint8_t data[2];
		data[0] = 0x03;
		data[1] = MODES[mode_chooser.get_active()].value;
		message.reset(new MRFDongle::SendReliableMessageOperation(dongle, index, data, sizeof(data)));
		message->signal_done.connect(sigc::mem_fun(this, &LEDsPanel::check_result));
	}
}

void LEDsPanel::check_result(AsyncOperation<void> &op) {
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
	message.reset();
	set_button.set_sensitive();
}

