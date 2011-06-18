#include "ai/hl/stp/evaluation/ball.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam posses_threshold("circle radius in front of robot to consider possesion (meters)", "STP/ball", 0.1, 0.0, 1.0);
}

bool AI::HL::STP::Evaluation::posses_ball(const World &world, Player::CPtr player, const Point target){
	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < posses_threshold;
}

