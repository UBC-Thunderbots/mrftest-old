#include "ai/backend/xbeed/player.h"
#include "geom/angle.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace AI::BE::XBeeD;

namespace {
	const unsigned int MAX_DRIBBLER_SPEED = 40000;
	const unsigned int BATTERY_CRITICAL_THRESHOLD = 12000;
	const unsigned int MAX_DRIBBLE_STALL_MILLISECONDS = 2000;
	const unsigned int DRIBBLE_RECOVER_TIME = 1000;
	const unsigned int CHICKER_MIN_INTERVAL = 5500;

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

#warning why the hell is this number so small?
const unsigned int Player::CHICKER_FOREVER = 1000;

void Player::move(Point dest, double target_ori, unsigned int flags, AI::Flags::MOVE_TYPE type, AI::Flags::MOVE_PRIO prio) {
	if (std::isnan(dest.x) || std::isnan(dest.y)) {
		destination_.first = position();
		LOG_WARN("NaN destination passed to player::move");
	} else {
		destination_.first = dest;
	}

	if (std::isnan(target_ori)) {
		destination_.second = orientation();
		LOG_WARN("NaN orientation passed to player::move");
	} else {
		destination_.second = target_ori;
	}

	flags_ = flags;
	move_type_ = type;
	move_prio_ = prio;

	moved = true;
}

const std::pair<Point, double> &Player::destination() const {
	return destination_;
}

void Player::dribble(double speed) {
	new_dribble_power = clamp(static_cast<int>(speed * 1023.0 + copysign(0.5, speed)), -1023, 1023);
}

unsigned int Player::chicker_ready_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, chicker_last_fire_time, diff);
	unsigned int millis = timespec_to_millis(diff);
	if (millis < CHICKER_MIN_INTERVAL) {
		return CHICKER_MIN_INTERVAL - millis;
	} else if (!bot->alive()) {
		return CHICKER_FOREVER;
	} else if (!bot->chicker_ready()) {
		return CHICKER_FOREVER;
	} else {
		return 0;
	}
}

void Player::kick(double power) {
	std::cout << pattern() << " kick(" << power << ")\n";
	if (bot->alive()) {
		if (!chicker_ready_time()) {
			unsigned int width = kicker_power_to_pulse_width(power);
			if (width > 0) {
				bot->kick(width);
			}
			timespec_now(chicker_last_fire_time);
		} else {
			chick_when_not_ready_message.activate(true);
		}
	}
}

void Player::chip(double power) {
	std::cout << pattern() << " chip(" << power << ")\n";
	if (bot->alive()) {
		if (!chicker_ready_time()) {
			unsigned int width = chipper_power_to_pulse_width(power);
			if (width > 0) {
				bot->chip(width);
			}
			timespec_now(chicker_last_fire_time);
		} else {
			chick_when_not_ready_message.activate(true);
		}
	}
}

double Player::sense_ball_time() const {
	if (sense_ball_) {
		timespec now;
		timespec_now(now);
		timespec diff;
		timespec_sub(now, sense_ball_start, diff);
		return timespec_to_double(diff);
	} else {
		return 0.0;
	}
}

double Player::last_sense_ball_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, sense_ball_end, diff);
	return timespec_to_double(diff);
}

void Player::dribbler_safety() {
	if (dribble_stall) {
		timespec now;
		timespec_now(now);
		timespec diff;
		timespec_sub(now, stall_start, diff);
		unsigned int milliseconds = timespec_to_millis(diff);
		if (milliseconds > MAX_DRIBBLE_STALL_MILLISECONDS) {
			recover_time_start = now;
		}
	}
}

bool Player::dribbler_safe() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, recover_time_start, diff);
	unsigned int milliseconds = timespec_to_millis(diff);
	return milliseconds > DRIBBLE_RECOVER_TIME;
}

Player::Ptr Player::create(AI::BE::Backend &backend, unsigned int pattern, XBeeDriveBot::Ptr bot) {
	Ptr p(new Player(backend, pattern, bot));
	return p;
}

Player::Player(AI::BE::Backend &backend, unsigned int pattern, XBeeDriveBot::Ptr bot) : AI::BE::XBeeD::Robot(backend, pattern), bot(bot), destination_(Point(), 0.0), moved(false), controlled(false), new_dribble_power(0), old_dribble_power(0), sense_ball_(0), dribble_stall(false), theory_dribble_rpm(0), dribble_distance_(0.0), not_moved_message(Glib::ustring::compose("Bot %1 not moved", pattern)), chick_when_not_ready_message(Glib::ustring::compose("Bot %1 chick when not ready", pattern)), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_LOW) {
	bot->signal_feedback.connect(sigc::mem_fun(this, &Player::on_feedback));
	timespec now;
	timespec_now(now);
	sense_ball_start = now;
	sense_ball_end = now;
	stall_start = now;
	recover_time_start = now;
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
	// This message may have been set earlier, but need not be set any more.
	chick_when_not_ready_message.activate(false);

	// Annunciate that we weren't moved if we have a Strategy but it never set a destination.
	not_moved_message.activate(bot->alive() && !scram && !moved);

	// Emergency conditions that cause scram of all systems.
	if (!bot->alive() || scram || bot->battery_voltage() < BATTERY_CRITICAL_THRESHOLD) {
		moved = false;
		new_dribble_power = 0;
	}

	// Drivetrain and chicker control path.
	if (moved && controlled) {
		bot->drive_controlled(wheel_speeds_[0], wheel_speeds_[1], wheel_speeds_[2], wheel_speeds_[3]);
		bot->enable_chicker(true);
		if (sense_ball_ >= HAS_BALL_TIME) {
			new_dribble_power = calc_dribble(wheel_speeds_, new_dribble_power);
		}
	} else {
		if (bot->alive()) {
			bot->drive_scram();
			bot->enable_chicker(false);
		}
	}
	moved = false;
	controlled = false;

	// Dribbler control path.
	new_dribble_power = dribbler_safe() ? new_dribble_power : 0;
	if (new_dribble_power) {
		bot->dribble(new_dribble_power);
		old_dribble_power = new_dribble_power;
		new_dribble_power = 0;
	} else {
		if (bot->alive()) {
			bot->dribble(0);
		}
		old_dribble_power = new_dribble_power = 0;
	}

	// Timestamp the robot to notify XBeeD that we're alive and slightly sane.
	if (bot->alive()) {
		bot->stamp();
	}

	// Calculations.
	if (sense_ball()) {
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

void Player::on_feedback() {
	theory_dribble_rpm = static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED);
	const unsigned int threshold_speed = static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED * DRIBBLER_HAS_BALL_LOAD_FACTOR);
	bool new_sense_ball = (bot->dribbler_speed() > 0) && (theory_dribble_rpm > 0) && (bot->dribbler_speed() < threshold_speed);
	if (new_sense_ball) {
		++sense_ball_;
		timespec_now(sense_ball_end);
		if (!sense_ball_) {
			timespec_now(sense_ball_start);
		}
	} else {
		sense_ball_ = 0;
	}
	// std::cout << "player: " << name << " sense_ball=" << sense_ball_ << std::endl;

	const bool stall = (theory_dribble_rpm > 0) && (bot->dribbler_speed() < 50);
	if (stall && !dribble_stall) {
		timespec_now(stall_start);
	}

	dribble_stall = stall;
}

