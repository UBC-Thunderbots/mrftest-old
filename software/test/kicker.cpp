#include "test/kicker.h"

namespace {
	Glib::ustring format_kick_pulse_value(double d) {
		return Glib::ustring::format(static_cast<int>(d) / 32 * 32);
	}
}

KickerPanel::KickerPanel(XBeeRobot::Ptr robot) : Gtk::Table(6, 2), robot(robot), charge("Charge"), pulse_width1_label("Pulse width 1:"), pulse_width2_label("Pulse width 2:"), pulse_offset_label("Offset:"), kick("Kick"), autokick("Autokick"), autokick_count_label("Autokick Count:"), autokick_count_value_label("0"), autokick_count(0) {
	robot->alive.signal_changed().connect(sigc::mem_fun(this, &KickerPanel::on_alive_changed));
	robot->signal_autokick_fired.connect(sigc::mem_fun(this, &KickerPanel::on_autokick_fired));

	charge.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_charge_changed));
	attach(charge, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_width1.get_adjustment()->configure(0, 0, 4064, 32, 256, 0);
	pulse_width1.set_digits(0);
	pulse_width1.signal_format_value().connect(&format_kick_pulse_value);
	attach(pulse_width1_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_width1, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_width2.get_adjustment()->configure(0, 0, 4064, 32, 256, 0);
	pulse_width2.set_digits(0);
	pulse_width2.signal_format_value().connect(&format_kick_pulse_value);
	attach(pulse_width2_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_width2, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_offset.get_adjustment()->configure(0, -4064, 4064, 32, 256, 0);
	pulse_offset.set_digits(0);
	pulse_offset.signal_format_value().connect(&format_kick_pulse_value);
	attach(pulse_offset_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_offset, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	kick.signal_clicked().connect(sigc::mem_fun(this, &KickerPanel::on_kick));
	fire_hbox.pack_start(kick, Gtk::PACK_EXPAND_WIDGET);
	autokick.signal_toggled().connect(sigc::mem_fun(this, &KickerPanel::on_autokick_changed));
	fire_hbox.pack_start(autokick, Gtk::PACK_EXPAND_WIDGET);
	attach(fire_hbox, 0, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	attach(autokick_count_label, 0, 1, 5, 6, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(autokick_count_value_label, 1, 2, 5, 6, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	on_alive_changed();
}

void KickerPanel::scram() {
	charge.set_active(false);
	autokick.set_active(false);
}

void KickerPanel::fire() {
	unsigned int pw1 = static_cast<unsigned int>(pulse_width1.get_value());
	unsigned int pw2 = static_cast<unsigned int>(pulse_width2.get_value());
	int offset = static_cast<int>(pulse_offset.get_value());
	robot->kick(pw1, pw2, offset);
}

void KickerPanel::on_alive_changed() {
	kick.set_sensitive(robot->alive);
}

void KickerPanel::on_charge_changed() {
	robot->enable_charger(charge.get_active());
}

void KickerPanel::on_kick() {
	fire();
}

void KickerPanel::on_autokick_changed() {
	if (autokick.get_active()) {
		unsigned int pw1 = static_cast<unsigned int>(pulse_width1.get_value());
		unsigned int pw2 = static_cast<unsigned int>(pulse_width2.get_value());
		int offset = static_cast<int>(pulse_offset.get_value());
		robot->autokick(pw1, pw2, offset);
	} else {
		robot->autokick(0, 0, 0);
	}
}

void KickerPanel::on_autokick_fired() {
	++autokick_count;
	autokick_count_value_label.set_text(Glib::ustring::format(autokick_count));
}

