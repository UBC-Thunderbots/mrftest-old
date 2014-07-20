#include "test/common/kicker.h"
#include <gtkmm/adjustment.h>

KickerPanel::KickerPanel(Drive::Robot &robot) : Gtk::Table(5, 2), robot(robot), discharge_button(charge_group, u8"Discharge"), float_button(charge_group, u8"Float"), charge_button(charge_group, u8"Charge"), kicker_button(solenoid_group, u8"Kicker"), chipper_button(solenoid_group, u8"Chipper"), pulse_width_label(u8"Pulse width (µs):"), kick(u8"Kick"), autokick(u8"Autokick"), autokick_count_label(u8"Autokick Count:"), autokick_count_value_label(u8"0"), autokick_count(0) {
	robot.alive.signal_changed().connect(sigc::mem_fun(this, &KickerPanel::on_alive_changed));
	robot.signal_autokick_fired.connect(sigc::mem_fun(this, &KickerPanel::on_autokick_fired));

	float_button.set_active();
	discharge_button.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_charge_changed));
	float_button.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_charge_changed));
	charge_button.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_charge_changed));
	charge_box.pack_start(discharge_button, Gtk::PACK_EXPAND_WIDGET);
	charge_box.pack_start(float_button, Gtk::PACK_EXPAND_WIDGET);
	charge_box.pack_start(charge_button, Gtk::PACK_EXPAND_WIDGET);
	attach(charge_box, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	kicker_button.set_active();
	solenoid_box.pack_start(kicker_button, Gtk::PACK_EXPAND_WIDGET);
	solenoid_box.pack_start(chipper_button, Gtk::PACK_EXPAND_WIDGET);
	attach(solenoid_box, 0, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_width.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &KickerPanel::on_pulse_width_changed));
	pulse_width.get_adjustment()->configure(0, 0, robot.kick_pulse_maximum(), robot.kick_pulse_resolution(), robot.kick_pulse_resolution() * 100, 0);
	pulse_width.set_digits(2);
	attach(pulse_width_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_width, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	kick.signal_clicked().connect(sigc::mem_fun(this, &KickerPanel::on_kick));
	fire_hbox.pack_start(kick, Gtk::PACK_EXPAND_WIDGET);
	autokick.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_autokick_changed));
	fire_hbox.pack_start(autokick, Gtk::PACK_EXPAND_WIDGET);
	attach(fire_hbox, 0, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	attach(autokick_count_label, 0, 1, 4, 5, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(autokick_count_value_label, 1, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	on_alive_changed();
}

void KickerPanel::scram() {
	discharge_button.set_active();
	autokick.set_active(false);
}

void KickerPanel::fire() {
	robot.kick(chipper_button.get_active(), pulse_width.get_value());
}

void KickerPanel::on_alive_changed() {
	update_sensitive();
}

void KickerPanel::on_charge_changed() {
	if (float_button.get_active()) {
		robot.set_charger_state(Drive::Robot::ChargerState::FLOAT);
	} else if (charge_button.get_active()) {
		robot.set_charger_state(Drive::Robot::ChargerState::CHARGE);
	} else {
		robot.set_charger_state(Drive::Robot::ChargerState::DISCHARGE);
	}
}

void KickerPanel::on_pulse_width_changed() {
	update_sensitive();
}

void KickerPanel::on_kick() {
	fire();
}

void KickerPanel::on_autokick_changed() {
	kicker_button.set_sensitive(!autokick.get_active());
	chipper_button.set_sensitive(!autokick.get_active());
	if (autokick.get_active()) {
		robot.autokick(chipper_button.get_active(), pulse_width.get_value());
	} else {
		robot.autokick(false, 0);
	}
}

void KickerPanel::on_autokick_fired() {
	// Update and display the counter.
	++autokick_count;
	autokick_count_value_label.set_text(Glib::ustring::format(autokick_count));

	// Autokick is implicitly disarmed once it has fired; rearm it.
	on_autokick_changed();
}

void KickerPanel::update_sensitive() {
	bool pulse_ok = pulse_width.get_value() >= 1.0;
	kick.set_sensitive(robot.alive && pulse_ok);
	autokick.set_sensitive(pulse_ok);
}

