#include "ai/world/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
	const unsigned int MAX_DRIBBLER_SPEED = 40000;
	const double DRIBBLER_HAS_BALL_LOAD_FACTOR = 0.75;
	const unsigned int BATTERY_CRITICAL_THRESHOLD = 12000;
	const unsigned int MAX_DRIBBLE_STALL_MILLISECONDS = 2000;
	const unsigned int DRIBBLE_RECOVER_TIME = 1000;
	const unsigned int CHICKER_MIN_INTERVAL = 10000;

	//const double HAS_BALL_TIME = 2.0 / 15.0;
	const int HAS_BALL_TIME = 3;
	const int HAS_BALL_THRESH = 3;
	const int HAS_BALL_SAMPLE = 5;

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
		point x_y;

		for (unsigned int i = 0; i < 4; ++i) {
			theta += wheel_speeds[i];
			point speed(0.0, wheel_speeds[i]);
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

const double player::MAX_DRIBBLE_DIST = 0.30;

player::state::~state() {
}

uint64_t player::address() const {
	return bot->address;
}

void player::move(const point &dest, double target_ori) {
	if (std::isnan(dest.x) || std::isnan(dest.y)) {
		destination_ = position();
		LOG("NaN destination passed to player::move");
	} else {
		destination_ = dest;
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
	new_dribble_power = clamp(static_cast<int>(speed * 1023.0 + 0.5), 0, 1023);
}

unsigned int player::chicker_ready_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, chicker_last_fire_time, diff);
	unsigned int millis = timespec_to_millis(diff);
	if (millis < CHICKER_MIN_INTERVAL) {
		return CHICKER_MIN_INTERVAL - millis;
	} else if (!bot->alive()) {
		return 1000;
	} else if (!bot->chicker_ready()) {
		return 1000;
	} else {
		return 0;
	}
}

void player::kick(double power) {
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

void player::chip(double power) {
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

double player::sense_ball_time() const {
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

double player::last_sense_ball_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, sense_ball_end, diff);
	return timespec_to_double(diff);
}

void player::dribbler_safety() {
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

bool player::dribbler_safe() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, recover_time_start, diff);
	unsigned int milliseconds = timespec_to_millis(diff);
	return milliseconds > DRIBBLE_RECOVER_TIME;
}

bool player::has_ball() const {
	return sense_ball_ >= HAS_BALL_TIME;
	// return sense_ball() && sense_ball_time() >= HAS_BALL_TIME;
}

player::state::ptr player::get_state(const std::type_info &tid) const {
	std::map<const std::type_info *, state::ptr>::const_iterator i = state_store.find(&tid);
	return i != state_store.end() ? i->second : state::ptr();
}

void player::set_state(const std::type_info &tid, player::state::ptr state) {
	if (state) {
		state_store[&tid] = state;
	} else {
		state_store.erase(&tid);
	}
}

player::ptr player::create(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) {
	ptr p(new player(name, yellow, pattern_index, bot));
	return p;
}

player::player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) : robot(yellow, pattern_index), name(name), bot(bot), target_orientation(0.0), moved(false), new_dribble_power(0), old_dribble_power(0), sense_ball_(0), theory_dribble_rpm(0), dribble_distance_(0.0), state_store(&compare_type_infos), not_moved_message(Glib::ustring::compose("%1 not moved", name)), chick_when_not_ready_message(Glib::ustring::compose("%1 chick when not ready", name)) {
	bot->signal_feedback.connect(sigc::mem_fun(this, &player::on_feedback));
	timespec now;
	timespec_now(now);
	sense_ball_end = now;
	sense_ball_start = now;
	chicker_last_fire_time.tv_sec = 0;
	chicker_last_fire_time.tv_nsec = 0;
}

void player::tick(bool scram) {
	// This message may have been set earlier, but need not be set any more.
	chick_when_not_ready_message.activate(false);

	// Annunciate that we weren't moved if we have a strategy but it never set a
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
		if (has_ball()) {
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

void player::on_feedback() {
	theory_dribble_rpm =  static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED);
	const unsigned int threshold_speed = static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED * DRIBBLER_HAS_BALL_LOAD_FACTOR);
	const bool new_sense_ball = bot->dribbler_speed() < threshold_speed;
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

