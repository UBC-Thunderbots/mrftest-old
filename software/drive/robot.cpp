#include "drive/robot.h"

Drive::Robot::~Robot() = default;

Drive::Robot::Robot(unsigned int index) : index(index), alive(false), ball_in_beam(false), capacitor_charged(false), battery_voltage(0), capacitor_voltage(0), break_beam_reading(0), dribbler_temperature(0), board_temperature(0) {
}

