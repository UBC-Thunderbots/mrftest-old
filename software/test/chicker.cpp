#include "test/chicker.h"

namespace {
	Glib::ustring format_chick_pulse_value(double d) {
		return Glib::ustring::format(static_cast<int>(d) / 32 * 32);
	}
}

ChickerPanel::ChickerPanel(XBeeRobot::Ptr robot) : Gtk::Table(5, 2), robot(robot), charge("Charge"), pulse_width1_label("Pulse width 1:"), pulse_width2_label("Pulse width 2:"), pulse_offset_label("Offset:"), kick("Kick"), autokick("Autokick") {
	robot->alive.signal_changed().connect(sigc::mem_fun(this, &ChickerPanel::on_alive_changed));

	charge.signal_toggled().connect(sigc::mem_fun(this, &ChickerPanel::on_charge_changed));
	attach(charge, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_width1.get_adjustment()->configure(0, 0, 4064, 32, 256, 0);
	pulse_width1.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &ChickerPanel::on_pulse_width_changed));
	pulse_width1.set_digits(0);
	pulse_width1.signal_format_value().connect(&format_chick_pulse_value);
	attach(pulse_width1_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_width1, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_width2.get_adjustment()->configure(0, 0, 4064, 32, 256, 0);
	pulse_width2.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &ChickerPanel::on_pulse_width_changed));
	pulse_width2.set_digits(0);
	pulse_width2.signal_format_value().connect(&format_chick_pulse_value);
	attach(pulse_width2_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_width2, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	pulse_offset.get_adjustment()->configure(0, -4064, 4064, 32, 256, 0);
	pulse_offset.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &ChickerPanel::on_pulse_offset_changed));
	pulse_offset.set_digits(0);
	pulse_offset.signal_format_value().connect(&format_chick_pulse_value);
	attach(pulse_offset_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(pulse_offset, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	kick.signal_clicked().connect(sigc::mem_fun(this, &ChickerPanel::on_kick));
	fire_hbox.pack_start(kick, Gtk::PACK_EXPAND_WIDGET);
	autokick.signal_toggled().connect(sigc::mem_fun(this, &ChickerPanel::on_autokick_changed));
	fire_hbox.pack_start(autokick, Gtk::PACK_EXPAND_WIDGET);
	attach(fire_hbox, 0, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	on_alive_changed();
}

void ChickerPanel::scram() {
	charge.set_active(false);
	autokick.set_active(false);
}

void ChickerPanel::fire() {
	unsigned int pw1 = static_cast<unsigned int>(pulse_width1.get_value());
	unsigned int pw2 = static_cast<unsigned int>(pulse_width2.get_value());
	int offset = static_cast<int>(pulse_offset.get_value());
	robot->kick(pw1, pw2, offset);
}

void ChickerPanel::on_alive_changed() {
	kick.set_sensitive(robot->alive);
}

void ChickerPanel::on_charge_changed() {
	robot->enable_chicker(charge.get_active());
}

void ChickerPanel::on_pulse_width_changed() {
	if (pulse_offset.get_value() + pulse_width1.get_value() > 4064) {
		pulse_offset.set_value(4064 - pulse_width1.get_value());
	} else if (-pulse_offset.get_value() + pulse_width2.get_value() > 4064) {
		pulse_offset.set_value(-(4064 - pulse_width2.get_value()));
	}
	if (autokick.get_active()) {
		on_autokick_changed();
	}
}

void ChickerPanel::on_pulse_offset_changed() {
	if (pulse_offset.get_value() + pulse_width1.get_value() > 4064) {
		pulse_width1.set_value(4064 - pulse_offset.get_value());
	} else if (-pulse_offset.get_value() + pulse_width2.get_value() > 4064) {
		pulse_width2.set_value(4064 - -pulse_offset.get_value());
	}
	if (autokick.get_active()) {
		on_autokick_changed();
	}
}

void ChickerPanel::on_kick() {
	fire();
}

void ChickerPanel::on_autokick_changed() {
	unsigned int pw1 = static_cast<unsigned int>(pulse_width1.get_value());
	unsigned int pw2 = static_cast<unsigned int>(pulse_width2.get_value());
	int offset = static_cast<int>(pulse_offset.get_value());
	if (!autokick.get_active()) {
		pw1 = 0;
		pw2 = 0;
		offset = 0;
	}
	robot->autokick(pw1, pw2, offset);
}

