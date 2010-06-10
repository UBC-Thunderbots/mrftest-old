#include "ai/world/player.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>

namespace {
	const unsigned int MAX_DRIBBLER_SPEED = 40000;
	const double DRIBBLER_HAS_BALL_LOAD_FACTOR = 0.75;
	const unsigned int BATTERY_WARNING_THRESHOLD = 13500;
	const unsigned int BATTERY_NOWARNING_THRESHOLD = 14500;
	const unsigned int BATTERY_CRITICAL_THRESHOLD = 12000;
	const unsigned int BATTERY_WARNING_FILTER_TIME = 5000;
	const unsigned int MAX_DRIBBLE_STALL_MILLISECONDS = 2000;
	const unsigned int DRIBBLE_RECOVER_TIME = 1000;

	unsigned int chicker_power_to_pulse_width(double power) {
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
		static const int CONTINUOUS_IDLE_AMOUNT = 25;
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
			return std::max(new_dribble_power, -x);
		} else if (x_y.x > FORWARD_EXEMPTION_AMOUNT) {
			return CONTINUOUS_IDLE_AMOUNT;
		}
		return new_dribble_power;
	}
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

double player::sense_ball_time() const {
	if (sense_ball_) {
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		timespec diff;
		timespec_sub(now, sense_ball_start, diff);
		return diff.tv_sec + diff.tv_nsec / 1000000000.0;
	} else {
		return 0.0;
	}
}

double player::last_sense_ball_time() const {
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	timespec diff;
	timespec_sub(now, sense_ball_end, diff);
	return diff.tv_sec + diff.tv_nsec / 1000000000.0;
}

void player::dribbler_safety() {
	if (dribble_stall) {
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		timespec diff;
		timespec_sub(now, stall_start, diff);
		unsigned int milliseconds = diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
		if (milliseconds > MAX_DRIBBLE_STALL_MILLISECONDS) {
			recover_time_start = now;
		}
	}
}

bool player::dribbler_safe() const {
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	timespec diff;
	timespec_sub(now, recover_time_start, diff);
	unsigned int milliseconds = diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
	return milliseconds > DRIBBLE_RECOVER_TIME;
}

player::ptr player::create(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) {
	ptr p(new player(name, yellow, pattern_index, bot));
	return p;
}

player::player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) : robot(yellow, pattern_index), bot(bot), target_orientation(0.0), moved(false), new_dribble_power(0), old_dribble_power(0), sense_ball_(false), theory_dribble_rpm(0), dribble_distance_(0.0), low_battery_message(Glib::ustring::compose("%1 low battery", name)), chicker_fault_message(Glib::ustring::compose("%1 chicker fault", name)) {
	bot->signal_feedback.connect(sigc::mem_fun(this, &player::on_feedback));
	clock_gettime(CLOCK_MONOTONIC, &sense_ball_end);
	clock_gettime(CLOCK_MONOTONIC, &low_battery_start_time);
}

void player::tick(bool scram) {
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
		if (sense_ball()) {
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
	if (bot->battery_voltage() < BATTERY_WARNING_THRESHOLD) {
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		timespec diff;
		timespec_sub(now, low_battery_start_time, diff);
		if (diff.tv_sec * 1000U + diff.tv_nsec / 1000000U > BATTERY_WARNING_FILTER_TIME) {
			low_battery_message.activate(true);
		}
	} else {
		clock_gettime(CLOCK_MONOTONIC, &low_battery_start_time);
		if (bot->battery_voltage() > BATTERY_NOWARNING_THRESHOLD) {
			low_battery_message.activate(false);
		}
	}
	chicker_fault_message.activate(bot->chicker_faulted());

	theory_dribble_rpm =  static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED);
	unsigned int threshold_speed = static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED * DRIBBLER_HAS_BALL_LOAD_FACTOR);
	bool new_sense_ball = bot->dribbler_speed() < threshold_speed;
	if (new_sense_ball) {
		clock_gettime(CLOCK_MONOTONIC, &sense_ball_end);
		if (!sense_ball_) {
			clock_gettime(CLOCK_MONOTONIC, &sense_ball_start);
		}
	}

	bool stall = (theory_dribble_rpm > 0) && (bot->dribbler_speed() < 50);
	if (stall && !dribble_stall) {
		clock_gettime(CLOCK_MONOTONIC, &stall_start);
	}

	dribble_stall = stall;
	sense_ball_ = new_sense_ball;
}

