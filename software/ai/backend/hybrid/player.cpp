#include "ai/backend/hybrid/player.h"
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

using namespace AI::BE::Hybrid;

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

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot *xbee_bot, MRFRobot *mrf_bot) : AI::BE::Hybrid::Robot(backend, pattern), xbee_bot(xbee_bot), mrf_bot(mrf_bot), controlled(false), dribble_distance_(0.0), battery_warning_hysteresis(-BATTERY_HYSTERESIS_MAGNITUDE), battery_warning_message(Glib::ustring::compose("Bot %1 low battery", pattern), Annunciator::Message::TriggerMode::LEVEL), autokick_fired_(false) {
	assert(xbee_bot || mrf_bot);
	timespec now;
	timespec_now(now);
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	if (xbee_bot) {
		xbee_bot->signal_autokick_fired.connect(sigc::mem_fun(this, &Player::on_autokick_fired));
	}
	if (mrf_bot) {
		mrf_bot->signal_autokick_fired.connect(sigc::mem_fun(this, &Player::on_autokick_fired));
	}
}

Player::~Player() {
	if (xbee_bot) {
		xbee_bot->drive_scram();
		xbee_bot->dribble(false);
		xbee_bot->autokick(0, 0, 0);
		xbee_bot->set_charger_state(XBeeRobot::ChargerState::DISCHARGE);
	}
	if (mrf_bot) {
		mrf_bot->drive_coast();
		mrf_bot->dribble(false);
		mrf_bot->autokick(false, 0);
		mrf_bot->set_charger_state(MRFRobot::ChargerState::DISCHARGE);
	}
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
	if (xbee_bot) {
		switch (index) {
			case 0:
				return xbee_bot->alive ? clamp((xbee_bot->battery_voltage - 13.0) / (16.5 - 13.0), 0.0, 1.0) : 0.0;

			case 1:
				return xbee_bot->alive ? clamp(xbee_bot->capacitor_voltage / 230.0, 0.0, 1.0) : 0.0;

			default:
				throw std::logic_error("invalid bar graph index");
		}
	}
	if (mrf_bot) {
		switch (index) {
			case 0:
				return mrf_bot->alive ? clamp((mrf_bot->battery_voltage - 13.0) / (16.5 - 13.0), 0.0, 1.0) : 0.0;

			case 1:
				return mrf_bot->alive ? clamp(mrf_bot->capacitor_voltage / 230.0, 0.0, 1.0) : 0.0;

			default:
				throw std::logic_error("invalid bar graph index");
		}
	}
	std::abort();
}

Visualizable::Colour Player::bar_graph_colour(unsigned int index) const {
	switch (index) {
		case 0:
		{
			double value = bar_graph_value(index);
			return Visualizable::Colour(1.0 - value, value, 0.0);
		}

		case 1:
			if (xbee_bot) {
				return Visualizable::Colour(xbee_bot->capacitor_charged ? 0.0 : 1.0, xbee_bot->capacitor_charged ? 1.0 : 0.0, 0.0);
			}
			if (mrf_bot) {
				return Visualizable::Colour(mrf_bot->capacitor_charged ? 0.0 : 1.0, mrf_bot->capacitor_charged ? 1.0 : 0.0, 0.0);
			}

		default:
			throw std::logic_error("invalid bar graph index");
	}
	std::abort();
}

bool Player::alive() const {
	if (xbee_bot) {
		return xbee_bot->alive;
	}
	if (mrf_bot) {
		return mrf_bot->alive;
	}
	std::abort();
}

bool Player::has_ball() const {
	if (xbee_bot) {
		return xbee_bot->ball_in_beam;
	}
	if (mrf_bot) {
		return mrf_bot->ball_in_beam;
	}
	std::abort();
}

bool Player::chicker_ready() const {
	if (xbee_bot) {
		return xbee_bot->alive && xbee_bot->capacitor_charged;
	}
	if (mrf_bot) {
		return mrf_bot->alive && mrf_bot->capacitor_charged;
	}
	std::abort();
}

void Player::kick_impl(double speed) {
	if (xbee_bot) {
		if (xbee_bot->alive) {
			if (xbee_bot->capacitor_charged) {
				xbee_bot->kick(calc_kick_straight(speed), 0, 0);
			} else {
				LOG_ERROR(Glib::ustring::compose("Bot %1 kick when not ready", pattern()));
			}
		}
	}
	if (mrf_bot) {
		if (mrf_bot->alive) {
			if (mrf_bot->capacitor_charged) {
				mrf_bot->kick(false, static_cast<unsigned int>(speed / 8.0 * 3000.0 * 4.0));
			} else {
				LOG_ERROR(Glib::ustring::compose("Bot %1 kick when not ready", pattern()));
			}
		}
	}
}

void Player::autokick_impl(double speed) {
	if (xbee_bot) {
		autokick_params.chip = false;
		autokick_params.pulse = calc_kick_straight(speed);
	}
	if (mrf_bot) {
		autokick_params.chip = false;
		autokick_params.pulse = static_cast<unsigned int>(speed / 8.0 * 3000.0 * 4.0);
	}
}

void Player::chip_impl(double) {
	if (xbee_bot) {
		if (xbee_bot->alive) {
			if (xbee_bot->capacitor_charged) {
				xbee_bot->kick(0, 4000, 0);
			} else {
				LOG_ERROR(Glib::ustring::compose("Bot %1 chip when not ready", pattern()));
			}
		}
	}
	if (mrf_bot) {
		if (mrf_bot->alive) {
			if (mrf_bot->capacitor_charged) {
				mrf_bot->kick(true, 16000);
			} else {
				LOG_ERROR(Glib::ustring::compose("Bot %1 chip when not ready", pattern()));
			}
		}
	}
}

void Player::autochip_impl(double) {
	autokick_params.chip = true;
	if (xbee_bot) {
		autokick_params.pulse = 4000U;
	}
	if (mrf_bot) {
		autokick_params.pulse = 16000U;
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
	bool alive = false, has_feedback = false;
	double battery_voltage = 0.0;
	if (xbee_bot) {
		alive = xbee_bot->alive;
		has_feedback = xbee_bot->has_feedback;
		battery_voltage = xbee_bot->battery_voltage;
	}
	if (mrf_bot) {
		alive = mrf_bot->alive;
		has_feedback = mrf_bot->has_feedback;
		battery_voltage = mrf_bot->battery_voltage;
	}

	// Check for emergency conditions.
	if (!alive) {
		halt = true;
	}

	// Check for low battery condition.
	if (alive && has_feedback) {
		// Apply some hysteresis.
		if (battery_voltage < BATTERY_CRITICAL_THRESHOLD) {
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
		if (xbee_bot) {
			unsigned int pulse1 = autokick_params.chip ? 0 : autokick_params.pulse;
			unsigned int pulse2 = autokick_params.chip ? autokick_params.pulse : 0;
			xbee_bot->autokick(pulse1, pulse2, 0);
		}
		if (mrf_bot) {
			mrf_bot->autokick(autokick_params.chip, autokick_params.pulse);
		}
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
		if (xbee_bot) {
			xbee_bot->drive(wheel_speeds_);
		}
		if (mrf_bot) {
			mrf_bot->drive(wheel_speeds_);
		}
	} else {
		if (xbee_bot) {
			xbee_bot->drive_scram();
		}
		if (mrf_bot) {
			mrf_bot->drive_coast();
		}
	}
	controlled = false;

	// Dribbler should always run except in halt.
	if (xbee_bot) {
		xbee_bot->dribble(!halt);
	}
	if (mrf_bot) {
		mrf_bot->dribble(!halt);
	}

	// Kicker should always charge except in halt.
	if (xbee_bot) {
		xbee_bot->set_charger_state(halt ? XBeeRobot::ChargerState::DISCHARGE : XBeeRobot::ChargerState::CHARGE);
	}
	if (mrf_bot) {
		mrf_bot->set_charger_state(halt ? MRFRobot::ChargerState::DISCHARGE : MRFRobot::ChargerState::CHARGE);
	}

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

Player::AutokickParams::AutokickParams() : chip(false), pulse(0) {
}

bool Player::AutokickParams::operator==(const AutokickParams &other) const {
	return chip == other.chip && pulse == other.pulse;
}

bool Player::AutokickParams::operator!=(const AutokickParams &other) const {
	return !(*this == other);
}

