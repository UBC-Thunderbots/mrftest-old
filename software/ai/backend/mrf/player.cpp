#include "ai/backend/mrf/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/config.h"
#include "util/dprint.h"
#include "util/string.h"
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

using namespace AI::BE::MRF;

namespace {
	const double BATTERY_CRITICAL_THRESHOLD = 13.5;

	const int BATTERY_HYSTERESIS_MAGNITUDE = 15;
}

Player::Player(unsigned int pattern, MRFRobot &bot) : AI::BE::Player(pattern), bot(bot), dribble_distance_(0.0), battery_warning_hysteresis(-BATTERY_HYSTERESIS_MAGNITUDE), battery_warning_message(Glib::ustring::compose("Bot %1 low battery", pattern), Annunciator::Message::TriggerMode::LEVEL), autokick_fired_(false) {
	bot.signal_autokick_fired.connect(sigc::mem_fun(this, &Player::on_autokick_fired));
}

Player::~Player() {
	bot.drive_coast();
	bot.dribble(false);
	bot.autokick(false, 0);
	bot.set_charger_state(MRFRobot::ChargerState::DISCHARGE);
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

bool Player::has_ball() const {
	return bot.ball_in_beam;
}

bool Player::chicker_ready() const {
	return bot.alive && bot.capacitor_charged;
}

void Player::kick_impl(double speed) {
	if (bot.alive) {
		if (bot.capacitor_charged) {
			bot.kick(false, speed / 8.0 * 3000);
		} else {
			LOG_ERROR(Glib::ustring::compose("Bot %1 kick when not ready", pattern()));
		}
	}
}

void Player::autokick_impl(double speed) {
	if (bot.alive) {
		autokick_params.chip = false;
		autokick_params.pulse = speed / 8.0 * 3000;
	}
}

void Player::chip_impl(double) {
	if (bot.alive) {
		if (bot.capacitor_charged) {
			bot.kick(true, 4000);
		} else {
			LOG_ERROR(Glib::ustring::compose("Bot %1 chip when not ready", pattern()));
		}
	}
}

void Player::autochip_impl(double) {
	if (bot.alive) {
		autokick_params.chip = true;
		autokick_params.pulse = 4000;
	}
}

void Player::on_autokick_fired() {
	autokick_fired_ = true;
}

void Player::tick(bool halt) {
	// Check for emergency conditions.
	if (!bot.alive) {
		halt = true;
	}

	// Check for low battery condition.
	if (bot.alive) {
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

	// Auto-kick should be enabled in non-halt conditions.
	if (halt) {
		autokick_params.chip = false;
		autokick_params.pulse = 0;
	}

	// Only if the current request has changed or the system needs rearming is a packet needed.
	if ((autokick_params != autokick_params_old) || (autokick_params.pulse && autokick_fired_)) {
		bot.autokick(autokick_params.chip, autokick_params.pulse);
		autokick_params_old = autokick_params;
	}

	// For the next tick, the AI needs to call the function again to keep the mechanism armed.
	// Clear the current parameters here so that a disarm will occur at the end of the next tick if needed.
	autokick_params.chip = false;
	autokick_params.pulse = 0;

	// Clear the autokick flag so it doesn't stick at true forever.
	autokick_fired_ = false;

	// Drivetrain control path.
	if (!halt && moved && controlled) {
		bot.drive(wheel_speeds_);
	} else {
		bot.drive_brake();
	}
	controlled = false;

	// Dribbler should always run except in halt.
	bot.dribble(!halt);

	// Kicker should always charge except in halt.
	bot.set_charger_state(halt ? MRFRobot::ChargerState::DISCHARGE : MRFRobot::ChargerState::CHARGE);

	// Calculations.
	if (has_ball()) {
		dribble_distance_ += (position(0.0) - last_dribble_position).len();
	} else {
		dribble_distance_ = 0.0;
	}
	last_dribble_position = position(0.0);
}

Player::AutokickParams::AutokickParams() : chip(false), pulse(0) {
}

bool Player::AutokickParams::operator==(const AutokickParams &other) const {
	return chip == other.chip && pulse == other.pulse;
}

bool Player::AutokickParams::operator!=(const AutokickParams &other) const {
	return !(*this == other);
}

