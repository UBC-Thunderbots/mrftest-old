#include "test/feedback.h"
#include <iomanip>

TesterFeedbackPanel::TesterFeedbackPanel(XBeeDongle &dongle) : Gtk::Table(5, 2), dongle(dongle), estop("EStop Run"), ball_in_beam("Ball in Beam"), ball_on_dribbler("Ball on Dribbler"), capacitor_charged("Capacitor Charged") {
	attach(*Gtk::manage(new Gtk::Label("Battery:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(battery_voltage, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*Gtk::manage(new Gtk::Label("Capacitor:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(capacitor_voltage, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*Gtk::manage(new Gtk::Label("Dribbler:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribbler_temperature, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*Gtk::manage(new Gtk::Label("Break Beam:")), 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(break_beam_reading, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	estop.set_sensitive(false);
	ball_in_beam.set_sensitive(false);
	ball_on_dribbler.set_sensitive(false);
	capacitor_charged.set_sensitive(false);
	Gtk::Table *subtable = Gtk::manage(new Gtk::Table(1, 4));
	subtable->attach(estop, 0, 1, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	subtable->attach(ball_in_beam, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	subtable->attach(ball_on_dribbler, 2, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	subtable->attach(capacitor_charged, 3, 4, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*subtable, 0, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	dongle.estop_state.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_estop_changed));
	on_estop_changed();

	set_robot(XBeeRobot::Ptr());
}

TesterFeedbackPanel::~TesterFeedbackPanel() {
}

void TesterFeedbackPanel::set_robot(XBeeRobot::Ptr bot) {
	alive_connection.disconnect();
	robot = bot;
	if (robot.is()) {
		alive_connection = robot->alive.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_alive_changed));
	}
	on_alive_changed();
}

void TesterFeedbackPanel::on_estop_changed() {
	estop.set_active(dongle.estop_state == XBeeDongle::EStopState::RUN);
}

void TesterFeedbackPanel::on_alive_changed() {
	battery_voltage_connection.disconnect();
	capacitor_voltage_connection.disconnect();
	dribbler_temperature_connection.disconnect();
	break_beam_reading_connection.disconnect();
	ball_in_beam_connection.disconnect();
	ball_on_dribbler_connection.disconnect();
	capacitor_charged_connection.disconnect();

	if (robot.is() && robot->alive) {
		battery_voltage_connection = robot->battery_voltage.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_battery_voltage_changed));
		capacitor_voltage_connection = robot->capacitor_voltage.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_capacitor_voltage_changed));
		dribbler_temperature_connection = robot->dribbler_temperature.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_dribbler_temperature_changed));
		break_beam_reading_connection = robot->break_beam_reading.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_break_beam_reading_changed));
		ball_in_beam_connection = robot->ball_in_beam.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_ball_in_beam_changed));
		ball_on_dribbler_connection = robot->ball_on_dribbler.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_ball_on_dribbler_changed));
		capacitor_charged_connection = robot->capacitor_charged.signal_changed().connect(sigc::mem_fun(this, &TesterFeedbackPanel::on_capacitor_charged_changed));
	}

	on_battery_voltage_changed();
	on_capacitor_voltage_changed();
	on_dribbler_temperature_changed();
	on_break_beam_reading_changed();
	on_ball_in_beam_changed();
	on_ball_on_dribbler_changed();
	on_capacitor_charged_changed();
}

void TesterFeedbackPanel::on_battery_voltage_changed() {
	if (robot.is() && robot->alive && robot->has_feedback) {
		battery_voltage.set_fraction(robot->battery_voltage / 18.0);
		battery_voltage.set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), robot->battery_voltage)));
	} else {
		battery_voltage.set_fraction(0);
		battery_voltage.set_text("No Data");
	}
}

void TesterFeedbackPanel::on_capacitor_voltage_changed() {
	if (robot.is() && robot->alive && robot->has_feedback) {
		capacitor_voltage.set_fraction(robot->capacitor_voltage / 250.0);
		capacitor_voltage.set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(0), robot->capacitor_voltage)));
	} else {
		capacitor_voltage.set_fraction(0);
		capacitor_voltage.set_text("No Data");
	}
}

void TesterFeedbackPanel::on_dribbler_temperature_changed() {
	if (robot.is() && robot->alive && robot->has_feedback) {
		dribbler_temperature.set_fraction(robot->dribbler_temperature / 125.0);
		dribbler_temperature.set_text(Glib::ustring::compose("%1°C", Glib::ustring::format(std::fixed, std::setprecision(1), robot->dribbler_temperature)));
	} else {
		dribbler_temperature.set_fraction(0);
		dribbler_temperature.set_text("No Data");
	}
}

void TesterFeedbackPanel::on_break_beam_reading_changed() {
	if (robot.is() && robot->alive && robot->has_feedback) {
		break_beam_reading.set_fraction(robot->break_beam_reading / 1023.0);
		break_beam_reading.set_text(Glib::ustring::format(robot->break_beam_reading));
	} else {
		break_beam_reading.set_fraction(0);
		break_beam_reading.set_text("No Data");
	}
}

void TesterFeedbackPanel::on_ball_in_beam_changed() {
	ball_in_beam.set_active(robot.is() && robot->alive && robot->has_feedback && robot->ball_in_beam);
}

void TesterFeedbackPanel::on_ball_on_dribbler_changed() {
	ball_on_dribbler.set_active(robot.is() && robot->alive && robot->has_feedback && robot->ball_on_dribbler);
}

void TesterFeedbackPanel::on_capacitor_charged_changed() {
	capacitor_charged.set_active(robot.is() && robot->alive && robot->has_feedback && robot->capacitor_charged);
}

