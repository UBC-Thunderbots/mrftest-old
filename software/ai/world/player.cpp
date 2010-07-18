#include "ai/world/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#include "uicomponents/param.h"

namespace {
	const unsigned int MAX_DRIBBLER_SPEED = 40000;
	const unsigned int BATTERY_CRITICAL_THRESHOLD = 12000;
	const unsigned int MAX_DRIBBLE_STALL_MILLISECONDS = 2000;
	const unsigned int DRIBBLE_RECOVER_TIME = 1000;
	const unsigned int CHICKER_MIN_INTERVAL = 5500;

	DoubleParam DRIBBLER_HAS_BALL_LOAD_FACTOR("Has Ball Load Factor", 0.8, 0.1, 3.0);

	//const double HAS_BALL_TIME = 2.0 / 15.0;
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

	//determines how much dribbling to do based off the inputs to the 4 motors
	int calc_dribble(const int (&wheel_speeds)[4], int new_dribble_power) {
		// Angles in radians that the wheels are located off the forward direction
		static const double ANGLES[4] = {0.959931, 2.35619, 3.9269908, 5.32325}; 
		static const double BACKWARDS_SCALING_FACTOR = 4.0;
		//if we are moving with this little force forwards exempt the reduction of dribble speed
		static const double FORWARD_EXEMPTION_AMOUNT = 7.0;
		//we are expecting to idle the motor so just set the dribble motor to a low set-point
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

	bool compare_type_infos(const std::type_info *x, const std::type_info *y) {
		return x->before(*y);
	}
}

const unsigned int Player::CHICKER_FOREVER = 1000;

const double Player::MAX_DRIBBLE_DIST = 0.30;

Player::State::~State() {
}

uint64_t Player::address() const {
	return bot->address;
}

void Player::move(const Point &dest, double target_ori) {
	if (std::isnan(dest.x) || std::isnan(dest.y)) {
		destination_ = position();
		LOG_WARN("NaN destination passed to player::move");
	} else {
		destination_ = dest;
	}

	if (std::isnan(target_ori)) {
		target_orientation = orientation();
		LOG_WARN("NaN orientation passed to player::move");
	} else {
		target_orientation = target_ori;
	}

	moved = true;
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
	std::cout << name << " kick(" << power << ")\n";
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
	std::cout << name << " chip(" << power << ")\n";
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

Player::State::ptr Player::get_state(const std::type_info &tid) const {
	std::map<const std::type_info *, State::ptr>::const_iterator i = state_store.find(&tid);
	return i != state_store.end() ? i->second : State::ptr();
}

void Player::set_state(const std::type_info &tid, Player::State::ptr state) {
	if (state) {
		state_store[&tid] = state;
	} else {
		state_store.erase(&tid);
	}
}

Player::ptr Player::create(const Glib::ustring &name, bool yellow, unsigned int pattern_index, XBeeDriveBot::ptr bot) {
	ptr p(new Player(name, yellow, pattern_index, bot));
	return p;
}

Player::Player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, XBeeDriveBot::ptr bot) : Robot(yellow, pattern_index), name(name), bot(bot), target_orientation(0.0), moved(false), new_dribble_power(0), old_dribble_power(0), sense_ball_(0), theory_dribble_rpm(0), dribble_distance_(0.0), state_store(&compare_type_infos), not_moved_message(Glib::ustring::compose("%1 not moved", name)), chick_when_not_ready_message(Glib::ustring::compose("%1 chick when not ready", name)) {
	bot->signal_feedback.connect(sigc::mem_fun(this, &Player::on_feedback));
	timespec now;
	timespec_now(now);
	sense_ball_end = now;
	sense_ball_start = now;
	chicker_last_fire_time.tv_sec = 0;
	chicker_last_fire_time.tv_nsec = 0;
}

void Player::tick(bool scram) {
	// This message may have been set earlier, but need not be set any more.
	chick_when_not_ready_message.activate(false);

	// Annunciate that we weren't moved if we have a Strategy but it never set a
	// destination.
	not_moved_message.activate(bot->alive() && !scram && !moved);

	// Emergency conditions that cause scram of all systems.
	if (!bot->alive() || scram || !controller || bot->battery_voltage() < BATTERY_CRITICAL_THRESHOLD) {
		moved = false;
		new_dribble_power = 0;
	}

	// Drivetrain and chicker control path.
	if (moved) {
		int output[4];
		controller->move(destination_, target_orientation, output);
		for (unsigned int i = 0; i < 4; ++i) {
			output[i] = clamp(output[i], -1023, 1023);
		}
		bot->drive_controlled(output[0], output[1], output[2], output[3]);
		moved = false;
		bot->enable_chicker(true);
		if (sense_ball_ >= HAS_BALL_TIME) {
			new_dribble_power = calc_dribble(output, new_dribble_power);
		}
	} else {
		if (controller) {
			controller->clear();
		}
		if (bot->alive()) {
			bot->drive_scram();
			bot->enable_chicker(false);
		}
	}

	// Dribbler control path.
	new_dribble_power = dribbler_safe() ? new_dribble_power : 0;
	//std::cout << " dribble at " << new_dribble_power << std::endl;
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

void Player::on_feedback() {
	theory_dribble_rpm =  static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED);
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

