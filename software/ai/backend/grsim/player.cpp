#include "ai/backend/grsim/player.h"
#include "ai/backend/ball.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include <cmath>

using AI::BE::GRSim::Player;
using std::chrono::steady_clock;

namespace {
#warning The robot geometry here should be updated to match our own once we have a custom grSim INI file.
	constexpr steady_clock::duration CHICKER_CHARGE_TIME = std::chrono::seconds(2);
	constexpr double ROBOT_DISTANCE_TO_FRONT = 0.073;
	constexpr double BALL_DIAMETER = 0.043;
	constexpr double HAS_BALL_THRESHOLD = 0.01;
	constexpr double DRIBBLER_WIDTH = 0.08;
	constexpr double HALF_DRIBBLER_WIDTH = DRIBBLER_WIDTH / 2.0;
	const Angle HALF_DRIBBLER_ARC = Angle::atan(HALF_DRIBBLER_WIDTH / ROBOT_DISTANCE_TO_FRONT);
	constexpr Angle HALF_DRIBBLER_ARC_MARGIN = Angle::of_degrees(-2.0);
	constexpr double MAX_CHIP_SPEED = 3.603f;
	constexpr double CHIP_ANGLE = 51.483f;
	const double SINE_CHIP_ANGLE = std::sin(CHIP_ANGLE);
	const double COSINE_CHIP_ANGLE = std::cos(CHIP_ANGLE);
	constexpr double RADIANS_PER_SECOND__PER__QUARTER_DEGREES_PER_FIVE_MILLISECONDS = 200 /* 5ms / s */ * 0.25 /* degrees per quarter thereof */ * M_PI / 180.0 /* radians per degree */;
}

Player::Player(unsigned int pattern, const AI::BE::Ball &ball) :
		AI::BE::Player(pattern),
		ball(ball),
		dribble_requested(false),
		dribble_active(false),
		autokick_fired_(false),
		had_ball(false),
		chick_mode(ChickMode::IDLE),
		chick_power(0.0),
		last_chick_time(steady_clock::now()) {
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
#warning Write this for movement primitives
}

void Player::encode_orders(grSim_Robot_Command &packet) {
	packet.set_id(pattern());
	packet.set_veltangent(0.0);
	packet.set_velnormal(0.0);
	packet.set_velangular(0.0);
	packet.set_spinner(dribble_active);
	packet.set_wheelsspeed(true);

	autokick_fired_ = false;
#warning Write chicker handling for movement primitives.

#warning Write wheel handling for movement primitives.
}
