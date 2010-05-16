#include "ai/world/ball.h"

ball::ptr ball::create() {
	ptr p(new ball);
	return p;
}

ball::ball() : sign(1.0) {
}

void ball::update(const SSL_DetectionBall &packet) {
	const point new_pos(packet.x() / 1000.0 * sign, packet.y() / 1000.0 * sign);
	add_prediction_datum(new_pos, 0.0);
}

