#include "ai/backend/xbee/player.h"
#include "geom/angle.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace AI::BE::XBee;

namespace {
	const unsigned int MAX_DRIBBLER_SPEED = 40000;
	const double BATTERY_CRITICAL_THRESHOLD = 13.5;
	const unsigned int MAX_DRIBBLE_STALL_MILLISECONDS = 2000;
	const unsigned int DRIBBLE_RECOVER_TIME = 1000;
	const unsigned int CHICKER_MIN_INTERVAL = 2000;

	DoubleParam DRIBBLER_HAS_BALL_LOAD_FACTOR("Has Ball Load Factor", 0.8, 0.1, 3.0);

	// const double HAS_BALL_TIME = 2.0 / 15.0;
	const int HAS_BALL_TIME = 2;

	unsigned int kicker_power_to_pulse_width(double power) {
		static const unsigned int MAX_PULSE_WIDTH = 511;
		static const double MICROS_PER_TICK = 32;
		double millis = (std::log(1.0 - clamp(power, 0.0, 0.999)) - 0.8849) / -0.9197;
		return clamp(static_cast<unsigned int>(millis * 1000.0 / MICROS_PER_TICK + 0.5), 0U, MAX_PULSE_WIDTH);
	}

	unsigned int chipper_power_to_pulse_width(double power) {
		const unsigned int MAX_PULSE_WIDTH = 300;
		return clamp(static_cast<unsigned int>(MAX_PULSE_WIDTH * power), 0U, MAX_PULSE_WIDTH);
	}

	// determines how much dribbling to do based off the inputs to the 4 motors
	int calc_dribble(const int(&wheel_speeds)[4], int new_dribble_power) {
		// Angles in radians that the wheels are located off the forward direction
		static const double ANGLES[4] = { 0.959931, 2.35619, 3.9269908, 5.32325 };
		static const double BACKWARDS_SCALING_FACTOR = 4.0;
		// if we are moving with this little force forwards exempt the reduction of dribble speed
		static const double FORWARD_EXEMPTION_AMOUNT = 7.0;
		// we are expecting to idle the motor so just set the dribble motor to a low set-point
		static const int CONTINUOUS_IDLE_AMOUNT = 130;
		int theta;
		Point x_y;

		for (unsigned int i = 0; i < 4; ++i) {
			theta += wheel_speeds[i];
			Point speed(0.0, wheel_speeds[i]);
			speed = speed.rotate(ANGLES[i]);
			x_y.x += speed.x;
			x_y.y += speed.y;
		}

		if (x_y.x < 0) {
			int x = static_cast<int>(BACKWARDS_SCALING_FACTOR * x_y.x);
			return clamp(std::max(new_dribble_power, -x), 0, 1023);
		} else if (x_y.x > FORWARD_EXEMPTION_AMOUNT) {
			return CONTINUOUS_IDLE_AMOUNT;
		}
		return new_dribble_power;
	}

	unsigned int calc_kick(double speed) {
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
		return static_cast<unsigned int>(clamp(power, 0.0, 4094.0));
	}
}

void Player::move_impl(Point dest, double target_ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	destination_.first = dest;
	destination_.second = target_ori;
	target_velocity_ = vel;
	flags_ = flags;
	move_type_ = type;
	move_prio_ = prio;

	moved = true;
}

const std::pair<Point, double> &Player::destination() const {
	return destination_;
}

Point Player::target_velocity() const {
	return target_velocity_;
}

bool Player::has_ball() const {
	return bot->ball_in_beam;
}

bool Player::chicker_ready() const {
	return bot->alive && bot->capacitor_charged;
}

void Player::kick_impl(double speed) {
	if (bot->alive) {
		if (bot->capacitor_charged) {
			bot->kick(calc_kick(speed), 0, 0);
		} else {
			chick_when_not_ready_message.fire();
		}
	}
}

void Player::autokick_impl(double speed) {
	if (bot->alive) {
		bot->autokick(calc_kick(speed), 0, 0);
	}
}

Player::Ptr Player::create(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) {
	Ptr p(new Player(backend, pattern, bot));
	return p;
}

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) : AI::BE::XBee::Robot(backend, pattern), bot(bot), destination_(Point(), 0.0), moved(false), controlled(false), dribble_distance_(0.0), chick_when_not_ready_message(Glib::ustring::compose("Bot %1 chick when not ready", pattern), Annunciator::Message::TRIGGER_EDGE), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_LOW), autokick_invoked(false) {
	timespec now;
	timespec_now(now);
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
}

void Player::drive(const int(&w)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		wheel_speeds_[i] = w[i];
	}
	controlled = true;
}

void Player::tick(bool halt) {
	// Check for emergency conditions.
	if (!bot->alive || bot->battery_voltage < BATTERY_CRITICAL_THRESHOLD) {
		halt = true;
	}

	// Inhibit auto-kick if halted or if the AI didn't renew its interest.
	if (halt || !autokick_invoked) {
		bot->autokick(0, 0, 0);
	}
	autokick_invoked = false;

	// Drivetrain control path.
	if (!halt && moved && controlled) {
		bot->drive(wheel_speeds_[0], wheel_speeds_[1], wheel_speeds_[2], wheel_speeds_[3]);
	} else {
		bot->drive_scram();
	}
	moved = false;
	controlled = false;

	// Dribbler should always run except in halt.
	bot->dribble(!halt);

	// Chicker should always charge except in halt.
	bot->enable_chicker(!halt);

	// Calculations.
	if (bot->ball_on_dribbler) {
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

