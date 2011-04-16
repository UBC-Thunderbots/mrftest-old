#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using AI::BE::Backend;
using AI::BE::BackendFactory;

AI::BE::Player::Player() : moved(false), destination_(Point(), 0.0), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_MEDIUM) {
}

void AI::BE::Player::move(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	move(dest, ori, vel);
	this->flags(flags);
	this->type(type);
	this->prio(prio);
}

void AI::BE::Player::move(Point dest, double ori, Point vel) {
	if (!std::isfinite(dest.x) || !std::isfinite(dest.y)) {
		LOG_ERROR("NaN or ±inf destination");
		dest = position(0);
	}
	if (!std::isfinite(ori)) {
		LOG_ERROR("NaN or ±inf orientation");
		ori = orientation(0);
	}
	if (!std::isfinite(vel.x) || !std::isfinite(vel.y)) {
		LOG_ERROR("NaN or ±inf velocity");
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

void AI::BE::Player::kick(double power) {
	if (!std::isfinite(power)) {
		LOG_ERROR("NaN or ±inf power");
		return;
	}
	if (power < 0) {
		LOG_ERROR("Out-of-range power");
		power = 0;
	}
	kick_impl(power);
}

void AI::BE::Player::autokick(double power) {
	if (!std::isfinite(power)) {
		LOG_ERROR("NaN or ±inf power");
		return;
	}
	if (power < 0) {
		LOG_ERROR("Out-of-range power");
		power = 0;
	}
	autokick_impl(power);
}

void AI::BE::Player::path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) {
	for (std::vector<std::pair<std::pair<Point, double>, timespec> >::const_iterator i = p.begin(), iend = p.end(); i != iend; ++i) {
		if (!std::isfinite(i->first.first.x) || !std::isfinite(i->first.first.y)) {
			LOG_ERROR("NaN or ±inf position in path element");
			return;
		}
		if (!std::isfinite(i->first.second)) {
			LOG_ERROR("NaN or ±inf orientation in path element");
			return;
		}
	}
	path_impl(p);
}

void AI::BE::Player::pre_tick() {
	moved = false;
	flags_ = 0;
	move_type_ = AI::Flags::MOVE_NORMAL;
	move_prio_ = AI::Flags::PRIO_MEDIUM;
}

Backend::Backend() : defending_end_(WEST), friendly_colour_(AI::Common::Team::YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::COUNT), ball_filter_(0) {
}

Backend::~Backend() {
}

void Backend::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	signal_draw_overlay_.emit(ctx);
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

BackendFactory::~BackendFactory() {
}

