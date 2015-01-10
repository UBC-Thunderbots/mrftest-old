#include "ai/backend/physical/player.h"
#include "drive/dongle.h"
#include "drive/robot.h"
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
#include <glibmm/ustring.h>

using AI::BE::Physical::Player;

Player::Player(unsigned int pattern, Drive::Robot &bot) :
		AI::BE::Player(pattern),
		bot(bot),
		robot_dead_message(Glib::ustring::compose(u8"Bot %1 dead", pattern), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		autokick_fired_(false),
		dribble_mode_(DribbleMode::STOP) {
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	bot.signal_autokick_fired.connect(sigc::mem_fun(this, &Player::on_autokick_fired));
}

Player::~Player() {
	bot.drive_coast();
	bot.dribble(0U);
	bot.autokick(false, 0);
	bot.set_charger_state(Drive::Robot::ChargerState::DISCHARGE);
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
			bot.kick(false, speed * 337.85 + 418.19) // new function, may be broken
			//bot.kick(false, speed / 8.0 * 825.0 * 4);
		} else {
			LOG_ERROR(Glib::ustring::compose(u8"Bot %1 kick when not ready", pattern()));
		}
	}
}

void Player::autokick_impl(double speed) {
	if (bot.alive) {
		autokick_params.chip = false;
		autokick_params.pulse = speed * 337.85 + 418.19 // new function, may be broken
		//autokick_params.pulse = speed / 8.0 * 825.0 * 4;
	}
}

void Player::chip_impl(double power) {
	if (bot.alive) {
		if (bot.capacitor_charged) {
			bot.kick(true, 800 * power * 4);
		} else {
			LOG_ERROR(Glib::ustring::compose(u8"Bot %1 chip when not ready", pattern()));
		}
	}
}

void Player::autochip_impl(double power) {
	if (bot.alive) {
		autokick_params.chip = true;
		autokick_params.pulse = 800 * power * 4;
	}
}

void AI::BE::Physical::Player::dribble(DribbleMode mode) {
	dribble_mode_ = mode;
}

void Player::on_autokick_fired() {
	autokick_fired_ = true;
}

void Player::tick(bool halt, bool stop) {
	// Show a message if the robot is dead.
	robot_dead_message.active(!bot.alive);

	// Check for emergency conditions.
	if (!bot.alive || bot.dongle().estop_state != Drive::Dongle::EStopState::RUN) {
		halt = true;
	}

	// Auto-kick should be enabled in non-halt conditions.
	if (halt) {
		autokick_params.chip = false;
		autokick_params.pulse = 0;
	}

	// Only if the current request has changed or the system needs rearming is a packet needed.
	if ((autokick_params != autokick_params_old) || (autokick_params.pulse != 0.0 && autokick_fired_)) {
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
		std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	}
	controlled = false;

	// Dribbler should always run except in halt or stop or when asked not to.
	if (halt || stop) {
		dribble_mode_ = DribbleMode::STOP;
	}
	double dribble_fraction = 0.0;
	switch (dribble_mode_) {
		case DribbleMode::STOP: dribble_fraction = 0.0; break;
		case DribbleMode::CATCH: dribble_fraction = 0.70; break;
		case DribbleMode::INTERCEPT: dribble_fraction = 0.33; break;
		case DribbleMode::CARRY: dribble_fraction = 0.33; break;
	}
	bot.dribble(static_cast<unsigned int>(dribble_fraction * bot.dribble_max_power));
	dribble_mode_ = DribbleMode::STOP;

	// Kicker should always charge except in halt.
	bot.set_charger_state(halt ? Drive::Robot::ChargerState::FLOAT : Drive::Robot::ChargerState::CHARGE);
}

Player::AutokickParams::AutokickParams() : chip(false), pulse(0) {
}

bool Player::AutokickParams::operator==(const AutokickParams &other) const {
	return chip == other.chip && pulse == other.pulse;
}

bool Player::AutokickParams::operator!=(const AutokickParams &other) const {
	return !(*this == other);
}

