#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

#warning const-correctness is broken in AI::BE::Robot::Ptr.

using AI::BE::Backend;
using AI::BE::BackendFactory;

AI::BE::Robot::Robot() : avoid_distance_(AI::Flags::AvoidDistance::MEDIUM) {
}

void AI::BE::Robot::avoid_distance(AI::Flags::AvoidDistance dist) const {
	avoid_distance_ = dist;
}

AI::Flags::AvoidDistance AI::BE::Robot::avoid_distance() const {
	return avoid_distance_;
}

void AI::BE::Robot::pre_tick() {
	avoid_distance_ = AI::Flags::AvoidDistance::MEDIUM;
}

void AI::BE::Player::move(Point dest, Angle ori, Point vel) {
	if (!std::isfinite(dest.x) || !std::isfinite(dest.y)) {
		LOG_ERROR("NaN or ±∞ destination");
		dest = position(0);
	}
	if (!ori.isfinite()) {
		LOG_ERROR("NaN or ±∞ orientation");
		ori = orientation(0);
	}
	if (!std::isfinite(vel.x) || !std::isfinite(vel.y)) {
		LOG_ERROR("NaN or ±∞ velocity");
		vel.x = vel.y = 0;
	}
	moved = true;
	destination_.first = dest;
	destination_.second = ori;
	target_velocity_ = vel;
}

void AI::BE::Player::flags(unsigned int flags) {
	if (flags & ~AI::Flags::FLAGS_VALID) {
		LOG_ERROR(Glib::ustring::compose("Invalid flag(s) 0x%08X", flags & ~AI::Flags::FLAGS_VALID));
		flags &= AI::Flags::FLAGS_VALID;
	}
	flags_ = flags;
}

void AI::BE::Player::type(AI::Flags::MoveType type) {
	move_type_ = type;
}

void AI::BE::Player::prio(AI::Flags::MovePrio prio) {
	move_prio_ = prio;
}

void AI::BE::Player::kick(double speed, Angle angle) {
	if (!std::isfinite(speed)) {
		LOG_ERROR("NaN or ±∞ speed");
		return;
	}
	if (speed < 0) {
		LOG_ERROR("Out-of-range speed");
		speed = 0;
	}
	if (!angle.isfinite()) {
		LOG_ERROR("NaN or ±∞ speed");
		return;
	}
	if (!kicker_directional() && angle.abs() > Angle::of_radians(1e-9)) {
		LOG_ERROR("Angled kick requested on nondirectional kicker");
		return;
	}
	kick_impl(speed, angle);
}

void AI::BE::Player::autokick(double speed, Angle angle) {
	if (!std::isfinite(speed)) {
		LOG_ERROR("NaN or ±∞ speed");
		return;
	}
	if (speed < 0) {
		LOG_ERROR("Out-of-range speed");
		speed = 0;
	}
	if (!angle.isfinite()) {
		LOG_ERROR("NaN or ±∞ speed");
		return;
	}
	if (!kicker_directional() && angle.abs() > Angle::of_radians(1e-9)) {
		LOG_ERROR("Angled kick requested on nondirectional kicker");
		return;
	}
	autokick_impl(speed, angle);
}

void AI::BE::Player::path(const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &p) {
	for (auto i = p.begin(), iend = p.end(); i != iend; ++i) {
		if (!std::isfinite(i->first.first.x) || !std::isfinite(i->first.first.y)) {
			LOG_ERROR("NaN or ±∞ position in path element");
			return;
		}
		if (!i->first.second.isfinite()) {
			LOG_ERROR("NaN or ±∞ orientation in path element");
			return;
		}
	}
	path_impl(p);
}

AI::BE::Player::Player() : moved(false), destination_(Point(), Angle::ZERO), flags_(0), move_type_(AI::Flags::MoveType::NORMAL), move_prio_(AI::Flags::MovePrio::MEDIUM) {
}

void AI::BE::Player::pre_tick() {
	AI::BE::Robot::pre_tick();
	moved = false;
	flags_ = 0;
	move_type_ = AI::Flags::MoveType::NORMAL;
	move_prio_ = AI::Flags::MovePrio::MEDIUM;
}

Backend::Backend() : defending_end_(FieldEnd::WEST), friendly_colour_(AI::Common::Team::Colour::YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::NONE), ball_filter_(0) {
}

void Backend::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	signal_draw_overlay_.emit(ctx);
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

