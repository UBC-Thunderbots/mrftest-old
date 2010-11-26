#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using AI::BE::Backend;
using AI::BE::BackendFactory;

void AI::BE::Player::move(Point dest, double ori, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	if (!std::isfinite(dest.x) || !std::isfinite(dest.y)) {
		LOG_WARN("NaN or ±inf destination in Player::move");
		dest = position(0);
	}
	if (!std::isfinite(ori)) {
		LOG_WARN("NaN or ±inf orientation in Player::move");
		ori = orientation(0);
	}
	move_impl(dest, ori, flags, type, prio);
}

void AI::BE::Player::kick(double power) {
	if (!std::isfinite(power)) {
		LOG_WARN("NaN or ±inf power in Player::kick");
		return;
	}
	if (power < 0 || power > 1) {
		LOG_WARN("Out-of-range power in Player::kick");
		power = clamp(power, 0.0, 1.0);
	}
	kick_impl(power);
}

void AI::BE::Player::chip(double power) {
	if (!std::isfinite(power)) {
		LOG_WARN("NaN or ±inf power in Player::chip");
		return;
	}
	if (power < 0 || power > 1) {
		LOG_WARN("Out-of-range power in Player::chip");
		power = clamp(power, 0.0, 1.0);
	}
	chip_impl(power);
}

void AI::BE::Player::path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) {
	for (std::vector<std::pair<std::pair<Point, double>, timespec> >::const_iterator i = p.begin(), iend = p.end(); i != iend; ++i) {
		if (!std::isfinite(i->first.first.x) || !std::isfinite(i->first.first.y)) {
			LOG_WARN("NaN or ±inf position in path element in Player::path");
			return;
		}
		if (!std::isfinite(i->first.second)) {
			LOG_WARN("NaN or ±inf orientation in path element in Player::path");
			return;
		}
	}
	path_impl(p);
}

void Backend::strategy(AI::HL::StrategyFactory *s) {
	if (s) {
		strategy_ = s->create_strategy(*this);
	} else {
		strategy_ = AI::HL::Strategy::Ptr();
	}
}

Backend::Backend() : defending_end_(WEST), friendly_colour_(YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::COUNT), ball_filter_(0), strategy_(AI::HL::Strategy::Ptr()) {
}

Backend::~Backend() {
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

BackendFactory::~BackendFactory() {
}

