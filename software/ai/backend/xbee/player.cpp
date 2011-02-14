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
}

void Player::move_impl(Point dest, double target_ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	destination_.first = dest;
	destination_.second = target_ori;
	target_velocity_ = Point();
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

unsigned int Player::chicker_ready_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, chicker_last_fire_time, diff);
	unsigned int millis = timespec_to_millis(diff);
	if (millis < CHICKER_MIN_INTERVAL) {
		return CHICKER_MIN_INTERVAL - millis;
	} else if (!bot->alive) {
		return 10000;
	} else if (!bot->capacitor_charged) {
		return 1;
	} else {
		return 0;
	}
}

void Player::kick_impl(double speed) {
	static const double SPEEDS[] = { 7.14, 8.89, 10.3 };
	static const unsigned int POWERS[] = { 2016, 3024, 4032 };

	if (bot->alive) {
		if (bot->capacitor_charged) {
			double speed_below, speed_above;
			unsigned int power_below, power_above;
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
			LOG_INFO(Glib::ustring::compose("kick_impl kicking %1 us", power));
			bot->kick(static_cast<unsigned int>(clamp(power, 0.0, 65535.0)));
			timespec_now(chicker_last_fire_time);
		} else {
			chick_when_not_ready_message.fire();
		}
	}
}

Player::Ptr Player::create(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) {
	Ptr p(new Player(backend, pattern, bot));
	return p;
}

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot) : AI::BE::XBee::Robot(backend, pattern), bot(bot), destination_(Point(), 0.0), moved(false), controlled(false), dribble_distance_(0.0), not_moved_message(Glib::ustring::compose("Bot %1 not moved", pattern), Annunciator::Message::TRIGGER_EDGE), chick_when_not_ready_message(Glib::ustring::compose("Bot %1 chick when not ready", pattern), Annunciator::Message::TRIGGER_EDGE), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_LOW) {
	timespec now;
	timespec_now(now);
	chicker_last_fire_time.tv_sec = 0;
	chicker_last_fire_time.tv_nsec = 0;
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
}

void Player::drive(const int(&w)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		wheel_speeds_[i] = w[i];
	}
	controlled = true;
}

void Player::tick(bool scram) {
	// Annunciate that we weren't moved if we have a Strategy but it never set a destination.
	if (bot->alive && !scram && !moved) {
		not_moved_message.fire();
	}

	// Emergency conditions that cause scram of all systems.
	if (!bot->alive || scram || bot->battery_voltage < BATTERY_CRITICAL_THRESHOLD) {
		moved = false;
	}

	// Drivetrain and chicker control path.
	if (moved && controlled) {
		bot->drive(wheel_speeds_[0], wheel_speeds_[1], wheel_speeds_[2], wheel_speeds_[3]);
		bot->enable_chicker();
	} else {
		if (bot->alive) {
			bot->drive_scram();
			bot->enable_chicker(false);
		}
	}
	moved = false;
	controlled = false;

	// Dribbler control path.
	if (bot->alive) {
		bot->dribble();
	}

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

