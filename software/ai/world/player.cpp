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
	const unsigned int BATTERY_CRITICAL_THRESHOLD = 12000;

	unsigned int chicker_power_to_pulse_width(double power) {
		const unsigned int MAX_PULSE_WIDTH = 511;
		return clamp(static_cast<unsigned int>(MAX_PULSE_WIDTH * power), 0U, MAX_PULSE_WIDTH);
	}
        
        //determines how much dribbling to do based off the inputs to the 4 motors
        int calc_dribble(int wheel_speeds[4]){
	  	/// Angles in radians that the wheels are located off the forward direction
	  	const double ANGLES[4] = {0.959931, 2.35619, 3.9269908, 5.32325}; 
	  	int theta;
	  	point x_y(0.0,0.0);

		for(int i=0; i<4; i++){
	    		theta += wheel_speeds[i];
	    		point speed;
	    		speed.x = static_cast<double>(wheel_speeds[i]);
	    		speed = speed.rotate(ANGLES[i]);
	    		if(i<2){
	      			speed = speed.rotate(-M_PI/2.0);
	    		}else{
	      			speed = speed.rotate(M_PI/2.0);
	    		}
	    		x_y.x+=speed.x;
	    		x_y.y+=speed.y;
	  	}
	  	int x = static_cast<int>(x_y.x);
	  	return std::min(1023, std::max(0, -x));
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

double player::has_ball_time() const {
	if (has_ball_) {
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		timespec diff;
		timespec_sub(now, has_ball_start, diff);
		return diff.tv_sec + diff.tv_nsec / 1000000000.0;
	} else {
		return 0.0;
	}
}

player::ptr player::create(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) {
	ptr p(new player(yellow, pattern_index, bot));
	return p;
}

player::player(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot) : robot(yellow, pattern_index), bot(bot), target_orientation(0.0), moved(false), new_dribble_power(0), old_dribble_power(0), has_ball_(false), theory_dribble_rpm(0), dribble_distance_(0.0) {
	bot->signal_feedback.connect(sigc::mem_fun(this, &player::on_feedback));
}

void player::tick(bool scram) {
	if (!bot->alive() || scram || !controller || bot->battery_voltage() < BATTERY_CRITICAL_THRESHOLD) {
		if (controller) {
			controller->clear();
		}
		if (bot->alive()) {
			bot->drive_scram();
			bot->enable_chicker(false);
		}
		moved = false;
		new_dribble_power = 0;
	}
	if (moved) {
		int output[4];
		controller->move(destination_, target_orientation, output);
		int m1 = clamp(output[0], -1023, 1023);
		int m2 = clamp(output[1], -1023, 1023);
		int m3 = clamp(output[2], -1023, 1023);
		int m4 = clamp(output[3], -1023, 1023);
		bot->drive_controlled(m1, m2, m3, m4);
		moved = false;
		bot->enable_chicker(true);
		if (has_ball()){
		  	int m[4];
		  	m[0] = m1;
		  	m[1] = m2;
		  	m[2] = m3;
		  	m[3] = m4;
		  	new_dribble_power = std::max(new_dribble_power, calc_dribble(m));
		}
	}
	if (bot->alive()) {
		bot->dribble(new_dribble_power);
		old_dribble_power = new_dribble_power;
		new_dribble_power = 0;
	} else {
		old_dribble_power = new_dribble_power = 0;
	}
	if (has_ball()) {
		dribble_distance_ += (position() - last_dribble_position).len();
	} else {
		dribble_distance_ = 0.0;
	}
	last_dribble_position = position();
}

void player::on_feedback() {
	theory_dribble_rpm =  static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED);
	unsigned int threshold_speed = static_cast<unsigned int>(std::abs(old_dribble_power) / 1023.0 * MAX_DRIBBLER_SPEED * DRIBBLER_HAS_BALL_LOAD_FACTOR);
	bool new_has_ball = bot->dribbler_speed() < threshold_speed;
	if (new_has_ball && !has_ball_) {
		clock_gettime(CLOCK_MONOTONIC, &has_ball_start);
	}
	has_ball_ = new_has_ball;
}

