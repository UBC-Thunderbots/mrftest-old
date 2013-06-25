#include "ai/backend/grsim/player.h"
#include "ai/backend/ball.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include <cmath>

using AI::BE::GRSim::Player;
using std::chrono::steady_clock;

namespace {
#warning The robot geometry here should be updated to match our own once we have a custom grSim INI file.
	const steady_clock::duration CHICKER_CHARGE_TIME = std::chrono::seconds(2);
	const double ROBOT_DISTANCE_TO_FRONT = 0.073;
	const double BALL_DIAMETER = 0.043;
	const double HAS_BALL_THRESHOLD = 0.01;
	const double DRIBBLER_WIDTH = 0.08;
	const double HALF_DRIBBLER_WIDTH = DRIBBLER_WIDTH / 2.0;
	const Angle HALF_DRIBBLER_ARC = Angle::atan(HALF_DRIBBLER_WIDTH / ROBOT_DISTANCE_TO_FRONT);
	const Angle HALF_DRIBBLER_ARC_MARGIN = Angle::of_degrees(-2.0);
	const double MAX_CHIP_SPEED = 3.603f;
	const double CHIP_ANGLE = 51.483f;
	const double SINE_CHIP_ANGLE = std::sin(CHIP_ANGLE);
	const double COSINE_CHIP_ANGLE = std::cos(CHIP_ANGLE);
	const double RADIANS_PER_SECOND__PER__QUARTER_DEGREES_PER_FIVE_MILLISECONDS = 200 /* 5ms / s */ * 0.25 /* degrees per quarter thereof */ * M_PI / 180.0 /* radians per degree */;
}

Player::Player(unsigned int pattern, const AI::BE::Ball &ball) : AI::BE::Player(pattern), ball(ball), dribble(false), autokick_fired_(false), had_ball(false), chick_mode(ChickMode::IDLE), chick_power(0.0), last_chick_time(steady_clock::now()) {
}

void Player::dribble_slow() {
}

bool Player::has_ball() const {
	Point ball_contact_point = ball.position() - Point::of_angle(orientation(0.0)) * BALL_DIAMETER / 2.0;
	Angle contact_point_offset_angle = (ball_contact_point - position(0.0)).orientation().angle_diff(orientation(0.0));
	if (!(contact_point_offset_angle <= HALF_DRIBBLER_ARC + HALF_DRIBBLER_ARC_MARGIN)) {
		return false;
	}
	double distance_to_ball_contact_point = (ball_contact_point - position(0.0)).len();
	double projected_distance_to_centre_of_dribbler = distance_to_ball_contact_point * contact_point_offset_angle.cos();
	return projected_distance_to_centre_of_dribbler <= ROBOT_DISTANCE_TO_FRONT + HAS_BALL_THRESHOLD;
}

bool Player::chicker_ready() const {
	return steady_clock::now() - last_chick_time >= CHICKER_CHARGE_TIME;
}

bool Player::autokick_fired() const {
	return autokick_fired_;
}

void Player::tick(bool halt, bool stop) {
	if (halt) {
		std::fill_n(wheel_speeds_, 4, 0);
		dribble = false;
		chick_mode = ChickMode::IDLE;
		chick_power = 0.0;
	} else {
		dribble = !stop;
	}
}

void Player::encode_orders(grSim_Robot_Command &packet) {
	packet.set_id(pattern());
	packet.set_veltangent(0.0);
	packet.set_velnormal(0.0);
	packet.set_velangular(0.0);
	packet.set_spinner(dribble);
	packet.set_wheelsspeed(true);

	autokick_fired_ = false;
	if (chicker_ready()) {
		if (chick_mode == ChickMode::AUTOKICK || chick_mode == ChickMode::AUTOCHIP) {
			if (has_ball()) {
				had_ball = true;
			} else if (had_ball) {
				last_chick_time = steady_clock::now();
				autokick_fired_ = true;
				chick_mode = ChickMode::IDLE;
				chick_power = 0.0;
				had_ball = false;
			}
		} else {
			had_ball = false;
		}

		switch (chick_mode) {
			case ChickMode::IDLE:
				packet.set_kickspeedx(0.0f);
				packet.set_kickspeedz(0.0f);
				break;

			case ChickMode::KICK:
			case ChickMode::AUTOKICK:
				packet.set_kickspeedx(static_cast<float>(chick_power));
				packet.set_kickspeedz(0.0f);
				break;

			case ChickMode::CHIP:
			case ChickMode::AUTOCHIP:
				packet.set_kickspeedx(static_cast<float>(MAX_CHIP_SPEED * COSINE_CHIP_ANGLE * chick_power));
				packet.set_kickspeedz(static_cast<float>(MAX_CHIP_SPEED * SINE_CHIP_ANGLE * chick_power));
				break;
		}
	} else {
		packet.set_kickspeedx(0.0f);
		packet.set_kickspeedz(0.0f);
	}
	chick_mode = ChickMode::IDLE;

	static void (grSim_Robot_Command::* const WHEEL_SETTERS[4])(float) = {
		&grSim_Robot_Command::set_wheel1,
		&grSim_Robot_Command::set_wheel2,
		&grSim_Robot_Command::set_wheel3,
		&grSim_Robot_Command::set_wheel4,
	};
	for (unsigned int i = 0; i < 4; ++i) {
		(packet.*WHEEL_SETTERS[i])(static_cast<float>(wheel_speeds_[i] * RADIANS_PER_SECOND__PER__QUARTER_DEGREES_PER_FIVE_MILLISECONDS));
	}
}

void Player::kick_impl(double speed) {
	chick_mode = ChickMode::KICK;
	chick_power = clamp(speed, 0.0, 8.0);
	last_chick_time = steady_clock::now();
}

void Player::autokick_impl(double speed) {
	chick_mode = ChickMode::AUTOKICK;
	chick_power = clamp(speed, 0.0, 8.0);
}

void Player::chip_impl(double power) {
	chick_mode = ChickMode::CHIP;
	chick_power = clamp(power, 0.0, 1.0);
	last_chick_time = steady_clock::now();
}

void Player::autochip_impl(double power) {
	chick_mode = ChickMode::AUTOCHIP;
	chick_power = clamp(power, 0.0, 1.0);
}

