#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using AI::BE::Backend;
using AI::BE::BackendFactory;

void AI::BE::Player::move(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
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
	if (flags & ~AI::Flags::FLAGS_VALID) {
		LOG_ERROR(Glib::ustring::compose("Invalid flag(s) 0x%08X", flags & ~AI::Flags::FLAGS_VALID));
		flags &= AI::Flags::FLAGS_VALID;
	}
	if (type >= AI::Flags::MOVE_COUNT) {
		LOG_ERROR(Glib::ustring::compose("Invalid move type %d", type));
		type = AI::Flags::MOVE_NORMAL;
	}
	if (prio >= AI::Flags::PRIO_COUNT) {
		LOG_ERROR(Glib::ustring::compose("Invalid move priority %d", prio));
		prio = AI::Flags::PRIO_LOW;
	}
	move_impl(dest, ori, vel, flags, type, prio);
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

Backend::Backend() : defending_end_(WEST), friendly_colour_(YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::COUNT), ball_filter_(0) {
}

Backend::~Backend() {
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

BackendFactory::~BackendFactory() {
}

