#include "ai/world/robot.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>

const double robot::MAX_RADIUS = 0.09;

robot::robot(bool yellow, unsigned int pattern_index) : yellow(yellow), pattern_index(pattern_index), sign(1.0), vision_failures(0), seen_this_frame(false) {
}

robot::ptr robot::create(bool yellow, unsigned int pattern_index) {
	ptr p(new robot(yellow, pattern_index));
	return p;
}

void robot::update(const SSL_DetectionRobot &packet) {
	if (packet.has_orientation()) {
		const point new_pos(packet.x() / 1000.0 * sign, packet.y() / 1000.0 * sign);
		const double new_ori = angle_mod(packet.orientation() + (sign > 0 ? 0.0 : M_PI));
		add_prediction_datum(new_pos, new_ori);
	} else {
		LOG("Vision packet has robot with no orientation.");
	}
}

