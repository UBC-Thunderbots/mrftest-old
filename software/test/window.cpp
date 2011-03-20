#include "test/window.h"
#include "test/matrix_drive.h"
#include "test/permotor_drive.h"
#include <algorithm>
#include <cassert>
#include <functional>

namespace {
	Glib::ustring format_chick_pulse_value(double d) {
		return Glib::ustring::format(static_cast<int>(d) / 32 * 32);
	}
}

TesterWindow::TesterWindow(XBeeDongle &dongle) : dongle(dongle), bot_frame("Bot"), control_bot_button("Control"), feedback_frame("Feedback"), feedback_panel(dongle), drive_frame("Drive"), drive_widget(0), drive_zeroable(0), dribble_frame("Dribbler"), dribble_button("Run"), chicker_frame("Chicker"), chicker_table(5, 2), chicker_enabled("Enable"), chicker_pulse_width1_label("Pulse width 1:"), chicker_pulse_width2_label("Pulse width 2:"), chicker_pulse_offset_label("Offset:"), chicker_kick("Kick"), chicker_autokick("Autokick"), params_frame("Parameters") {
	set_title("Robot Tester");

	for (unsigned int i = 0; i <= 15; ++i) {
		bot_chooser.append_text(Glib::ustring::format(i));
	}
	bot_chooser.set_active_text(Glib::ustring::format(0));
	bot_hbox.pack_start(bot_chooser, Gtk::PACK_EXPAND_WIDGET);
	control_bot_button.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_control_toggled));
	bot_hbox.pack_start(control_bot_button, Gtk::PACK_SHRINK);
	bot_frame.add(bot_hbox);
	vbox.pack_start(bot_frame, Gtk::PACK_SHRINK);

	feedback_frame.add(feedback_panel);
	vbox.pack_start(feedback_frame, Gtk::PACK_SHRINK);

	drive_chooser.append_text("Halt");
	drive_chooser.append_text("Per-Motor Drive");
	drive_chooser.append_text("Matrix Drive");
	drive_chooser.set_active_text("Halt");
	drive_chooser.set_sensitive(false);
	drive_chooser.signal_changed().connect(sigc::mem_fun(this, &TesterWindow::drive_mode_changed));
	drive_box.pack_start(drive_chooser, Gtk::PACK_SHRINK);
	drive_frame.add(drive_box);
	vbox.pack_start(drive_frame, Gtk::PACK_SHRINK);

	dribble_button.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_dribble_toggled));
	dribble_button.set_sensitive(false);
	dribble_frame.add(dribble_button);
	vbox.pack_start(dribble_frame, Gtk::PACK_SHRINK);

	chicker_enabled.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_enable_change));
	chicker_enabled.set_sensitive(false);
	chicker_table.attach(chicker_enabled, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_pulse_width1.get_adjustment()->configure(0, 0, 4094, 32, 256, 0);
	chicker_pulse_width1.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_pulse_width_changed));
	chicker_pulse_width1.set_digits(0);
	chicker_pulse_width1.signal_format_value().connect(&format_chick_pulse_value);
	chicker_table.attach(chicker_pulse_width1_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_table.attach(chicker_pulse_width1, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_pulse_width2.get_adjustment()->configure(0, 0, 4094, 32, 256, 0);
	chicker_pulse_width2.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_pulse_width_changed));
	chicker_pulse_width2.set_digits(0);
	chicker_pulse_width2.signal_format_value().connect(&format_chick_pulse_value);
	chicker_table.attach(chicker_pulse_width2_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_table.attach(chicker_pulse_width2, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_pulse_offset.get_adjustment()->configure(0, -4094, 4094, 32, 256, 0);
	chicker_pulse_offset.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_pulse_offset_changed));
	chicker_pulse_offset.set_digits(0);
	chicker_pulse_offset.signal_format_value().connect(&format_chick_pulse_value);
	chicker_table.attach(chicker_pulse_offset_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_table.attach(chicker_pulse_offset, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_kick.signal_clicked().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_kick));
	chicker_kick.set_sensitive(false);
	chicker_fire_hbox.pack_start(chicker_kick, Gtk::PACK_EXPAND_WIDGET);
	chicker_autokick.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_chicker_autokick));
	chicker_autokick.set_sensitive(false);
	chicker_fire_hbox.pack_start(chicker_autokick, Gtk::PACK_EXPAND_WIDGET);
	chicker_table.attach(chicker_fire_hbox, 0, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	chicker_frame.add(chicker_table);
	vbox.pack_start(chicker_frame, Gtk::PACK_SHRINK);

	params_frame.add(params_panel);
	vbox.pack_start(params_frame, Gtk::PACK_SHRINK);

	vbox.pack_start(ann, Gtk::PACK_EXPAND_WIDGET);

	add(vbox);

	Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(this, &TesterWindow::key_snoop));

	show_all();
}

void TesterWindow::on_control_toggled() {
	drive_chooser.set_active_text("Halt");
	dribble_button.set_active(false);
	chicker_enabled.set_active(false);

	if (control_bot_button.get_active()) {
		unsigned int index = bot_chooser.get_active_row_number();
		bot = dongle.robot(index);
		bot_alive_changed_signal = bot->alive.signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_bot_alive_changed));
		bot_chooser.set_sensitive(false);
		feedback_panel.set_robot(bot);
		params_panel.set_robot(bot);
		on_bot_alive_changed();
	} else {
		drive_chooser.set_active_text("Halt");
		bot_chooser.set_sensitive(true);
		drive_chooser.set_sensitive(false);
		dribble_button.set_sensitive(false);
		chicker_enabled.set_sensitive(false);
		chicker_kick.set_sensitive(false);
		chicker_autokick.set_sensitive(false);
		chicker_autokick.set_active(false);
		params_panel.set_robot(XBeeRobot::Ptr());
		feedback_panel.set_robot(XBeeRobot::Ptr());
		bot_alive_changed_signal.disconnect();
		bot.reset();
	}
}

void TesterWindow::on_bot_alive_changed() {
	if (bot.is() && bot->alive) {
		drive_chooser.set_sensitive(true);
		dribble_button.set_sensitive(true);
		chicker_enabled.set_sensitive(true);
		chicker_kick.set_sensitive(true);
		chicker_autokick.set_sensitive(true);
	} else {
		drive_chooser.set_active_text("Halt");
		drive_chooser.set_sensitive(false);
		dribble_button.set_sensitive(false);
		chicker_enabled.set_sensitive(false);
		chicker_kick.set_sensitive(false);
		chicker_autokick.set_sensitive(false);
		chicker_autokick.set_active(false);
	}
}

void TesterWindow::on_bot_claim_failed_locked() {
	on_bot_claim_failed("This robot cannot be claimed because it is already claimed by another client.");
}

void TesterWindow::on_bot_claim_failed_resource() {
	on_bot_claim_failed("This robot cannot be claimed because there are insufficient available radio resources.");
}

void TesterWindow::on_bot_claim_failed(const Glib::ustring &message) {
	control_bot_button.set_active(false);
	Gtk::MessageDialog md(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	md.set_title("Thunderbots Tester");
	md.run();
}

int TesterWindow::key_snoop(Widget *, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
		// Z letter scrams the system.
		drive_chooser.set_active_text("Halt");
		dribble_button.set_active(false);
		chicker_enabled.set_active(false);
	} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_0)) {
		// Zero digit sets all controls to zero but does not scram things.
		if (drive_zeroable) {
			drive_zeroable->zero();
		}
		dribble_button.set_active(false);
	}
	return 0;
}

void TesterWindow::drive_mode_changed() {
	// Remove any UI control.
	if (drive_widget) {
		drive_box.remove(*drive_widget);
		delete drive_widget;
		drive_widget = 0;
		drive_zeroable = 0;
	}

	// Zero out the drive levels.
	if (bot.is()) {
		bot->drive_scram();
	}

	// Create the new controls.
	const Glib::ustring &cur = drive_chooser.get_active_text();
	if (cur == "Halt") {
		// No controls.
	} else if (cur == "Per-Motor Drive") {
		// Use the direct drive controls.
		TesterControlPerMotorDrive *drive = new TesterControlPerMotorDrive(bot);
		drive_widget = drive;
		drive_zeroable = drive;
	} else if (cur == "Matrix Drive") {
		// Use the matrix drive controls.
		TesterControlMatrixDrive *drive = new TesterControlMatrixDrive(bot);
		drive_widget = drive;
		drive_zeroable = drive;
	}

	// Add the new controls to the UI.
	if (drive_widget) {
		drive_box.pack_start(*drive_widget);
		drive_widget->show_all();
	}
}

void TesterWindow::on_dribble_toggled() {
	if (bot.is()) {
		bot->dribble(dribble_button.get_active());
	}
}

void TesterWindow::on_chicker_enable_change() {
	if (bot.is()) {
		bot->enable_chicker(chicker_enabled.get_active());
	}
}

void TesterWindow::on_chicker_pulse_width_changed() {
	if (chicker_pulse_offset.get_value() + chicker_pulse_width1.get_value() > 16383) {
		chicker_pulse_offset.set_value(16383 - chicker_pulse_width1.get_value());
	} else if (-chicker_pulse_offset.get_value() + chicker_pulse_width2.get_value() > 16383) {
		chicker_pulse_offset.set_value(-(16383 - chicker_pulse_width2.get_value()));
	}
	if (chicker_autokick.get_active()) {
		on_chicker_autokick();
	}
}

void TesterWindow::on_chicker_pulse_offset_changed() {
	if (chicker_pulse_offset.get_value() + chicker_pulse_width1.get_value() > 16383) {
		chicker_pulse_width1.set_value(16383 - chicker_pulse_offset.get_value());
	} else if (-chicker_pulse_offset.get_value() + chicker_pulse_width2.get_value() > 16383) {
		chicker_pulse_width2.set_value(16383 - -chicker_pulse_offset.get_value());
	}
	if (chicker_autokick.get_active()) {
		on_chicker_autokick();
	}
}

void TesterWindow::on_chicker_kick() {
	if (bot.is()) {
		unsigned int pulse_width1 = static_cast<unsigned int>(chicker_pulse_width1.get_value());
		unsigned int pulse_width2 = static_cast<unsigned int>(chicker_pulse_width2.get_value());
		int offset = static_cast<int>(chicker_pulse_offset.get_value());
		bot->kick(pulse_width1, pulse_width2, offset);
	}
}

void TesterWindow::on_chicker_autokick() {
	if (bot.is()) {
		unsigned int pulse_width1 = static_cast<unsigned int>(chicker_pulse_width1.get_value());
		unsigned int pulse_width2 = static_cast<unsigned int>(chicker_pulse_width2.get_value());
		int offset = static_cast<int>(chicker_pulse_offset.get_value());
		if (!chicker_autokick.get_active()) {
			pulse_width1 = 0;
			pulse_width2 = 0;
			offset = 0;
		}
		bot->autokick(pulse_width1, pulse_width2, offset);
	}
}

