#include "ai/world/ball.h"

using namespace AI;

const double Ball::RADIUS = 0.0215;

Ball::Ball() : sign(1.0) {
}

void Ball::update(const Point &pos) {
	const Point new_pos(pos.x * sign, pos.y * sign);
	add_prediction_datum(new_pos, 0.0);
}

