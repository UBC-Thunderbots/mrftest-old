#include "ai/world/ball.h"

ball::ptr ball::create() {
	ptr p(new ball);
	return p;
}

ball::ball() : sign(1.0) {
}

void ball::update(const point &pos) {
	const point new_pos(pos.x * sign, pos.y * sign);
	add_prediction_datum(new_pos, 0.0);
}

