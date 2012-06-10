#include "ai/backend/xbee/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/config.h"
#include "util/dprint.h"
#include "util/string.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

using namespace AI::BE::XBee;

namespace {
	const double BATTERY_CRITICAL_THRESHOLD = 13.5;

	const int BATTERY_HYSTERESIS_MAGNITUDE = 15;

	unsigned int calc_kick_straight(double speed) {
		static const double SPEEDS[] = { 7.14, 8.89, 10.3 };
		static const unsigned int POWERS[] = { 2016, 3024, 4032 };

		double speed_below = 0.0, speed_above = 0.0;
		unsigned int power_below = 0.0, power_above = 0.0;
		if (speed <= SPEEDS[0] + 1e-9) {
			speed_below = SPEEDS[0];
			speed_above = SPEEDS[1];
			power_below = POWERS[0];
			power_above = POWERS[1];
		} else {
			for (std::size_t i = 0; i < G_N_ELEMENTS(SPEEDS) - 1 && SPEEDS[i] < speed; ++i) {
				speed_below = SPEEDS[i];
				speed_above = SPEEDS[i + 1];
				power_below = POWERS[i];
				power_above = POWERS[i + 1];
			}
		}
		double diff_speed = speed_above - speed_below;
		double diff_power = power_above - power_below;
		double slope = diff_power / diff_speed;
		double power = (speed - speed_below) * slope + power_below;
		return static_cast<unsigned int>(clamp(power, 0.0, 4064.0));
	}
}

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot &bot) : AI::BE::XBee::Robot(backend, pattern), bot(bot), controlled(false), dribble_distance_(0.0), battery_warning_hysteresis(-BATTERY_HYSTERESIS_MAGNITUDE), battery_warning_message(Glib::ustring::compose("Bot %1 low battery", pattern), Annunciator::Message::TriggerMode::LEVEL), autokick_invoked(false), autokick_fired_(false) {
	timespec now;
	timespec_now(now);
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	bot.signal_autokick_fired.connect(sigc::mem_fun(this, &Player::on_autokick_fired));
}

Player::~Player() {
	bot.drive_scram();
	bot.dribble(false);
	bot.autokick(0, 0, 0);
	bot.set_charger_state(XBeeRobot::ChargerState::DISCHARGE);
}

const std::pair<Point, Angle> &Player::destination() const {
	return destination_;
}

Point Player::target_velocity() const {
	return target_velocity_;
}

unsigned int Player::num_bar_graphs() const {
	return 2;
}

double Player::bar_graph_value(unsigned int index) const {
	switch (index) {
		case 0:
			return bot.alive ? clamp((bot.battery_voltage - 13.0) / (16.5 - 13.0), 0.0, 1.0) : 0.0;

		case 1:
			return bot.alive ? clamp(bot.capacitor_voltage / 230.0, 0.0, 1.0) : 0.0;

		default:
			throw std::logic_error("invalid bar graph index");
	}
}

Visualizable::Colour Player::bar_graph_colour(unsigned int index) const {
	switch (index) {
		case 0:
		{
			double value = bar_graph_value(index);
			return Visualizable::Colour(1.0 - value, value, 0.0);
		}

		case 1:
			return Visualizable::Colour(bot.capacitor_charged ? 0.0 : 1.0, bot.capacitor_charged ? 1.0 : 0.0, 0.0);

		default:
			throw std::logic_error("invalid bar graph index");
	}
}

bool Player::alive() const {
	return bot.alive;
}

bool Player::has_ball() const {
	return bot.ball_in_beam;
}

bool Player::chicker_ready() const {
	return bot.alive && bot.capacitor_charged;
}

void Player::kick_impl(double speed) {
	if (bot.alive) {
		if (bot.capacitor_charged) {
			bot.kick(calc_kick_straight(speed), 0, 0);
		} else {
			LOG_ERROR(Glib::ustring::compose("Bot %1 kick when not ready", pattern()));
		}
	}
}

void Player::autokick_impl(double speed) {
	return kick_impl(speed);
	if (bot.alive) {
		bot.autokick(calc_kick_straight(speed), 0, 0);
		autokick_invoked = true;
	}
}

void Player::chip_impl(double speed) {
	if (bot.alive) {
		if (bot.capacitor_charged) {
			bot.kick(0, 4000, 0);
		} else {
			LOG_ERROR(Glib::ustring::compose("Bot %1 chip when not ready", pattern()));
		}
	}
}

void Player::autochip_impl(double speed) {
	return chip_impl(speed);
	if (bot.alive) {
		bot.autokick(0, 4000, 0);
		autokick_invoked = true;
	}
}

void Player::drive(const int(&w)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		wheel_speeds_[i] = w[i];
	}
	controlled = true;
}

void Player::on_autokick_fired() {
	autokick_fired_ = true;
}

void Player::tick(bool halt) {
	// Clear the autokick flag so it doesn't stick at true forever.
	autokick_fired_ = false;

	// Check for emergency conditions.
	if (!bot.alive) {
		halt = true;
	}

	// Check for low battery condition.
	if (bot.alive && bot.has_feedback) {
		// Apply some hysteresis.
		if (bot.battery_voltage < BATTERY_CRITICAL_THRESHOLD) {
			if (battery_warning_hysteresis == BATTERY_HYSTERESIS_MAGNITUDE) {
				battery_warning_message.active(true);
			} else {
				++battery_warning_hysteresis;
			}
		} else {
			if (battery_warning_hysteresis == -BATTERY_HYSTERESIS_MAGNITUDE) {
				battery_warning_message.active(false);
			} else {
				--battery_warning_hysteresis;
			}
		}
	} else {
		battery_warning_message.active(false);
	}

	// Inhibit auto-kick if halted or if the AI didn't renew its interest.
	if (halt || !autokick_invoked) {
		bot.autokick(0, 0, 0);
	}
	autokick_invoked = false;

	// Drivetrain control path.
	if (!halt && moved && controlled) {
		bot.drive(wheel_speeds_);
	} else {
		bot.drive_scram();
	}
	controlled = false;

	// Dribbler should always run except in halt.
	bot.dribble(!halt);

	// Kicker should always charge except in halt.
	bot.set_charger_state(halt ? XBeeRobot::ChargerState::DISCHARGE : XBeeRobot::ChargerState::CHARGE);

	// Calculations.
	if (has_ball()) {
		dribble_distance_ += (position() - last_dribble_position).len();
	} else {
		dribble_distance_ = 0.0;
	}
	last_dribble_position = position();
}

Visualizable::Colour Player::visualizer_colour() const {
	return Visualizable::Colour(0.0, 1.0, 0.0);
}

Glib::ustring Player::visualizer_label() const {
	return Glib::ustring::format(pattern());
}

bool Player::highlight() const {
	return has_ball();
}

Visualizable::Colour Player::highlight_colour() const {
	return Visualizable::Colour(1.0, 0.5, 0.0);
}

