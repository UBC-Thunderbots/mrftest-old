#include "ai/backend/player.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include "geom/point.h"
#include "geom/angle.h"
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

void Player::destination(std::pair<Point,Angle> dest){
	if (!std::isfinite(dest.first.x) || !std::isfinite(dest.first.y)) {
		LOG_ERROR(u8"NaN or ±∞ destination");
		dest.first = Point();
	}
	if (!dest.second.isfinite()) {
		LOG_ERROR(u8"NaN or ±∞ orientation");
		dest.second = Angle();
	}
	destination_ = dest;
	return;
}

Point AI::BE::Player::target_velocity() const {
	return target_velocity_;
}

bool Player::has_display_path() const {
	return true;
}

const std::vector<Point> &Player::display_path() const {
	return display_path_;
}

void Player::display_path(const std::vector<Point> &p) {
	display_path_ = p;
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
#warning This needs writing for movement primitives.
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
	hl_request.type = Drive::Primitive::STOP;
	hl_request.field_bool = true;
	hl_request.field_point = Point(0, 0);
	hl_request.field_angle = Angle::zero();
	hl_request.field_angle2 = Angle::zero();
	hl_request.field_double = 0;
	hl_request.care_angle = false;
}

