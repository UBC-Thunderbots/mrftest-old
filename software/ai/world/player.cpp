#include "ai/world/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>

namespace {
	unsigned int chicker_power_to_pulse_width(double power) {
		const unsigned int MAX_PULSE_WIDTH = 511;
		return clamp(static_cast<unsigned int>(MAX_PULSE_WIDTH * power), 0U, MAX_PULSE_WIDTH);
	}
}

void player::move(const point &dest, double target_ori) {
	if (std::isnan(dest.x) || std::isnan(dest.y)) {
		destination = position();
		LOG("NaN destination passed to player::move");
	} else {
		destination = dest;
	}

	if (std::isnan(target_ori)) {
		target_orientation = orientation();
		LOG("NaN orientation passed to player::move");
	} else {
		target_orientation = target_ori;
	}

	moved = true;
}

void player::dribble(double speed) {
	if (bot->alive()) {
		bot->dribble(std::min(1023.0, std::max(0.0, speed * 1023.0)));
	}
}

void player::kick(double power) {
	if (bot->alive()) {
		unsigned int width = chicker_power_to_pulse_width(power);
		if (width > 0) {
			bot->kick(width);
		}
	}
}

void player::chip(double power) {
	if (bot->alive()) {
		unsigned int width = chicker_power_to_pulse_width(power);
		if (width > 0) {
			bot->chip(width);
		}
	}
}

bool player::has_ball() const {
	return !!(bot->feedback().flags & xbeepacket::FEEDBACK_FLAG_HAS_BALL);
}

player::ptr player::create(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) {
	ptr p(new player(yellow, pattern_index, bot));
	return p;
}

player::player(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) : robot(yellow, pattern_index), bot(bot), target_orientation(0.0), moved(false) {
	bot->signal_alive.connect(sigc::mem_fun(this, &player::on_bot_alive));
}

void player::tick(bool scram) {
	if (!bot->alive() || scram || !controller) {
		if (controller) {
			controller->clear();
		}
		if (bot->alive()) {
			bot->drive_scram();
		}
		moved = false;
	} else if (moved) {
		point vel;
		double avel;
		controller->move(destination * sign, angle_mod(target_orientation * sign), vel, avel);
		static const double matrix[4][3] = {
			{-42.5995, 27.6645, 4.3175},
			{-35.9169, -35.9169, 4.3175},
			{35.9169, -35.9169, 4.3175},
			{42.5995, 27.6645, 4.3175}
		};
		double input[3] = {
			vel.x,
			vel.y,
			avel
		};
		double output[4] = {0, 0, 0, 0};
		for (unsigned int row = 0; row < 4; ++row)
			for (unsigned int col = 0; col < 3; ++col)
				output[row] += matrix[row][col] * input[col];
		int m1 = clamp(static_cast<int>(output[0]), -1023, 1023);
		int m2 = clamp(static_cast<int>(output[1]), -1023, 1023);
		int m3 = clamp(static_cast<int>(output[2]), -1023, 1023);
		int m4 = clamp(static_cast<int>(output[3]), -1023, 1023);
		bot->drive_controlled(m1, m2, m3, m4);
		moved = false;
	}
}

void player::on_bot_alive() {
#warning Should the AI control the charger instead?
	bot->enable_chicker(true);
}

