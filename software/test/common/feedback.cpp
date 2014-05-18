#include "test/common/feedback.h"
#include "util/algorithm.h"
#include <iomanip>

TesterFeedbackPanel::TesterFeedbackPanel(Drive::Dongle &dongle, Drive::Robot &robot) :
		Gtk::Table(10, 2),
		dongle(dongle),
		robot(robot),
		battery_voltage_label(u8"Battery:"),
		capacitor_voltage_label(u8"Capacitor:"),
		dribbler_temperature_label(u8"Dribbler:"),
		board_temperature_label(u8"Board:"),
		break_beam_reading_label(u8"Break Beam:"),
		lqi_label(u8"LQI:"),
		rssi_label(u8"RSSI:"),
		alive(u8"Alive"),
		estop(u8"EStop Run"),
		ball_in_beam(u8"Ball in Beam"),
		capacitor_charged(u8"Capacitor Charged") {
	attach(battery_voltage_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(battery_voltage, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(capacitor_voltage_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(capacitor_voltage, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribbler_temperature_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribbler_temperature, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribbler_speed, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(board_temperature_label, 0, 1, 4, 5, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(board_temperature, 1, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(break_beam_reading_label, 0, 1, 5, 6, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(break_beam_reading, 1, 2, 5, 6, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(lqi_label, 0, 1, 6, 7, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(lqi_reading, 1, 2, 6, 7, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(rssi_label, 0, 1, 7, 8, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(rssi_reading, 1, 2, 7, 8, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	alive.set_sensitive(false);
	estop.set_sensitive(false);
	ball_in_beam.set_sensitive(false);
	capacitor_charged.set_sensitive(false);
	cb_hbox1.pack_start(alive, Gtk::PACK_EXPAND_WIDGET);
	cb_hbox1.pack_start(estop, Gtk::PACK_EXPAND_WIDGET);
	attach(cb_hbox1, 0, 2, 8, 9, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	cb_hbox2.pack_start(ball_in_beam, Gtk::PACK_EXPAND_WIDGET);
	cb_hbox2.pack_start(capacitor_charged, Gtk::PACK_EXPAND_WIDGET);
	attach(cb_hbox2, 0, 2, 9, 10, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	robot.battery_voltage.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_battery_voltage_changed));
	robot.capacitor_voltage.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_capacitor_voltage_changed));
	robot.dribbler_temperature.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_dribbler_temperature_changed));
	robot.dribbler_speed.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_dribbler_speed_changed));
	robot.board_temperature.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_board_temperature_changed));
	robot.break_beam_reading.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_break_beam_reading_changed));
	robot.link_quality.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_lqi_changed));
	robot.received_signal_strength.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_rssi_changed));
	robot.alive.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_alive_changed));
	dongle.estop_state.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_estop_changed));
	robot.ball_in_beam.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_ball_in_beam_changed));
	robot.capacitor_charged.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_capacitor_charged_changed));

	on_battery_voltage_changed();
	on_capacitor_voltage_changed();
	on_dribbler_temperature_changed();
	on_dribbler_speed_changed();
	on_board_temperature_changed();
	on_break_beam_reading_changed();
	on_lqi_changed();
	on_rssi_changed();
	on_alive_changed();
	on_estop_changed();
	on_ball_in_beam_changed();
	on_capacitor_charged_changed();
}

void TesterFeedbackPanel::on_battery_voltage_changed() {
	if (robot.alive) {
		battery_voltage.set_fraction(clamp(robot.battery_voltage / 18.0, 0.0, 1.0));
		battery_voltage.set_text(Glib::ustring::compose(u8"%1V", Glib::ustring::format(std::fixed, std::setprecision(2), robot.battery_voltage)));
	} else {
		battery_voltage.set_fraction(0);
		battery_voltage.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_capacitor_voltage_changed() {
	if (robot.alive) {
		capacitor_voltage.set_fraction(clamp(robot.capacitor_voltage / 250.0, 0.0, 1.0));
		capacitor_voltage.set_text(Glib::ustring::compose(u8"%1V", Glib::ustring::format(std::fixed, std::setprecision(0), robot.capacitor_voltage)));
	} else {
		capacitor_voltage.set_fraction(0);
		capacitor_voltage.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_dribbler_temperature_changed() {
	if (robot.alive && 0 < robot.dribbler_temperature && robot.dribbler_temperature < 200) {
		dribbler_temperature.set_fraction(clamp(robot.dribbler_temperature / 125.0, 0.0, 1.0));
		dribbler_temperature.set_text(Glib::ustring::compose(u8"%1°C", Glib::ustring::format(std::fixed, std::setprecision(1), robot.dribbler_temperature)));
	} else {
		dribbler_temperature.set_fraction(0);
		dribbler_temperature.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_dribbler_speed_changed() {
	if (robot.alive) {
		dribbler_speed.set_fraction(clamp(robot.dribbler_speed / 50000.0, 0.0, 1.0));
		dribbler_speed.set_text(Glib::ustring::compose(u8"%1 rpm", Glib::ustring::format(robot.dribbler_speed)));
	} else {
		dribbler_speed.set_fraction(0);
		dribbler_speed.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_board_temperature_changed() {
	if (robot.alive && 0 < robot.board_temperature && robot.board_temperature < 200) {
		board_temperature.set_fraction(clamp(robot.board_temperature / 125.0, 0.0, 1.0));
		board_temperature.set_text(Glib::ustring::compose(u8"%1°C", Glib::ustring::format(std::fixed, std::setprecision(1), robot.board_temperature)));
	} else {
		board_temperature.set_fraction(0);
		board_temperature.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_break_beam_reading_changed() {
	if (robot.alive) {
		break_beam_reading.set_fraction(clamp(robot.break_beam_reading / robot.break_beam_scale, 0.0, 1.0));
		break_beam_reading.set_text(Glib::ustring::format(robot.break_beam_reading));
	} else {
		break_beam_reading.set_fraction(0);
		break_beam_reading.set_text(u8"No Data");
	}
}

void TesterFeedbackPanel::on_lqi_changed() {
	lqi_reading.set_fraction(robot.link_quality);
	lqi_reading.set_text(Glib::ustring::format(robot.link_quality));
}

void TesterFeedbackPanel::on_rssi_changed() {
	rssi_reading.set_fraction((robot.received_signal_strength + 90) / (90.0 - 35.0));
	rssi_reading.set_text(Glib::ustring::compose(u8"%1 dB", robot.received_signal_strength));
}

void TesterFeedbackPanel::on_alive_changed() {
	alive.set_active(robot.alive);
	on_battery_voltage_changed();
	on_capacitor_voltage_changed();
	on_dribbler_temperature_changed();
	on_dribbler_speed_changed();
	on_board_temperature_changed();
	on_break_beam_reading_changed();
}

void TesterFeedbackPanel::on_estop_changed() {
	estop.set_active(dongle.estop_state == Drive::Dongle::EStopState::RUN);
}

void TesterFeedbackPanel::on_ball_in_beam_changed() {
	ball_in_beam.set_active(robot.alive && robot.ball_in_beam);
}

void TesterFeedbackPanel::on_capacitor_charged_changed() {
	capacitor_charged.set_active(robot.alive && robot.capacitor_charged);
}

