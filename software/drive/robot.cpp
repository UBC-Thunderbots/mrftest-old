#include "drive/robot.h"
#include <sigc++/functors/mem_fun.h>

Drive::Robot::~Robot() = default;

Drive::Robot::Robot(unsigned int index, double break_beam_scale, double kick_speed_max, double chip_distance_max, unsigned int direct_dribbler_max) :
		index(index),
		break_beam_scale(break_beam_scale),
		kick_speed_max(kick_speed_max),
		kick_speed_resolution(1.0),
		chip_distance_max(chip_distance_max),
		chip_distance_resolution(1.0),
		direct_dribbler_max(direct_dribbler_max),
		alive(false),
		direct_control(false),
		ball_in_beam(false),
		capacitor_charged(false),
		battery_voltage(0),
		capacitor_voltage(0),
		break_beam_reading(0),
		dribbler_temperature(0),
		dribbler_speed(0),
		board_temperature(0),
		link_quality(0),
		received_signal_strength(-90),
		build_ids_valid(false),
		fw_build_id(0),
		fpga_build_id(0),
		primitive(Primitive::STOP),
		low_battery_message(Glib::ustring::compose(u8"Bot %1 low battery", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		high_board_temperature_message(Glib::ustring::compose(u8"Bot %1 board hot", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH) {
	alive.signal_changed().connect(sigc::mem_fun(this, &Robot::handle_alive_changed));
	battery_voltage.signal_changed().connect(sigc::mem_fun(this, &Robot::handle_battery_voltage_changed));
	board_temperature.signal_changed().connect(sigc::mem_fun(this, &Robot::handle_board_temperature_changed));
}

void Drive::Robot::handle_alive_changed() {
	if (!alive) {
		low_battery_message.active(false);
		high_board_temperature_message.active(false);
	}
}

void Drive::Robot::handle_battery_voltage_changed() {
	if (battery_voltage < 13.5) {
		low_battery_message.active(true);
	} else if (battery_voltage > 14.0) {
		low_battery_message.active(false);
	}
}

void Drive::Robot::handle_board_temperature_changed() {
	if (board_temperature > 90.0) {
		high_board_temperature_message.active(true);
	} else if (board_temperature < 80.0) {
		high_board_temperature_message.active(false);
	}
}
