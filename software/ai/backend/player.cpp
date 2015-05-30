#include "ai/backend/player.h"
#include "ai/robot_controller/robot_controller.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include <cmath>
#include <glibmm/ustring.h>

namespace {
	BoolParam kalman_control_inputs(u8"Enable Kalman control inputs", u8"AI/Backend", false);
}

using AI::BE::Player;

void Player::move(Point dest, Angle ori, Point vel) {
	if (!std::isfinite(dest.x) || !std::isfinite(dest.y)) {
		LOG_ERROR(u8"NaN or ±∞ destination");
		dest = position(0);
	}
	if (!ori.isfinite()) {
		LOG_ERROR(u8"NaN or ±∞ orientation");
		ori = orientation(0);
	}
	if (!std::isfinite(vel.x) || !std::isfinite(vel.y)) {
		LOG_ERROR(u8"NaN or ±∞ velocity");
		vel.x = vel.y = 0;
	}
	moved = true;
	destination_.first = dest;
	destination_.second = ori;
	target_velocity_ = vel;
}

void Player::flags(unsigned int flags) {
	if (flags & ~AI::Flags::FLAGS_VALID) {
		LOG_ERROR(Glib::ustring::compose(u8"Invalid flag(s) 0x%08X", flags & ~AI::Flags::FLAGS_VALID));
		flags &= AI::Flags::FLAGS_VALID;
	}
	flags_ = flags;
}

bool Player::has_destination() const {
	return true;
}

std::pair<Point, Angle> Player::destination() const {
	return destination_;
}

void Player::kick(double speed) {
	if (!std::isfinite(speed)) {
		LOG_ERROR(u8"NaN or ±∞ speed");
		return;
	}
	if (speed < 0) {
		LOG_ERROR(u8"Out-of-range speed");
		speed = 0;
	}
	kick_impl(speed);
}

void Player::autokick(double speed) {
	if (!std::isfinite(speed)) {
		LOG_ERROR(u8"NaN or ±∞ speed");
		return;
	}
	if (speed < 0) {
		LOG_ERROR(u8"Out-of-range speed");
		speed = 0;
	}
	autokick_impl(speed);
}

void Player::chip(double power) {
	if (!(0 <= power && power <= 1)) {
		LOG_ERROR(u8"Out-of-range power");
		power = clamp(power, 0.0, 1.0);
	}
	chip_impl(power);
}

void Player::autochip(double power) {
	if (!(0 <= power && power <= 1)) {
		LOG_ERROR(u8"Out-of-range power");
		power = clamp(power, 0.0, 1.0);
	}
	autochip_impl(power);
}

Point AI::BE::Player::target_velocity() const {
	return target_velocity_;
}

bool Player::has_path() const {
	return true;
}

const std::vector<std::pair<std::pair<Point, Angle>, AI::Timestamp>> &Player::path() const {
	return path_;
}

void Player::path(const std::vector<std::pair<std::pair<Point, Angle>, AI::Timestamp>> &p) {
	for (const std::pair<std::pair<Point, Angle>, AI::Timestamp> &i : p) {
		if (!std::isfinite(i.first.first.x) || !std::isfinite(i.first.first.y)) {
			LOG_ERROR(u8"NaN or ±∞ position in path element");
			return;
		}
		if (!i.first.second.isfinite()) {
			LOG_ERROR(u8"NaN or ±∞ orientation in path element");
			return;
		}
	}
	path_ = p;
}

void Player::pre_tick() {
	AI::BE::Robot::pre_tick();
	moved = false;
	flags_ = 0;
	move_type_ = AI::Flags::MoveType::NORMAL;
	move_prio_ = AI::Flags::MovePrio::MEDIUM;
}

void Player::update_predictor(AI::Timestamp ts) {
	if (kalman_control_inputs) {
		double prediction_vector[3] = { 0, 0, 0 };
		for (unsigned int row = 0; row < 3; ++row) {
			for (unsigned int col = 0; col < 4; ++col) {
				prediction_vector[row] += AI::RC::RobotController::WHEEL_MATRIX_PINV[row][col] * wheel_speeds_[col];
			}
		}
		add_control(Point(prediction_vector[0], prediction_vector[1]), Angle::of_radians(prediction_vector[2]), ts);
	}
}

Visualizable::Colour Player::visualizer_colour() const {
	return Visualizable::Colour(0.0, 1.0, 0.0);
}

bool Player::highlight() const {
	return has_ball();
}

Visualizable::Colour Player::highlight_colour() const {
	if (has_ball()) {
		return Visualizable::Colour(1.0, 0.5, 0.0);
	} else {
		return Visualizable::Colour(0.0, 0.0, 0.0);
	}
}

Player::Player(unsigned int pattern) : AI::BE::Robot(pattern), moved(false), destination_(Point(), Angle::zero()), flags_(0), move_type_(AI::Flags::MoveType::NORMAL), move_prio_(AI::Flags::MovePrio::MEDIUM) {
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
}

