#include "ai/world/robot.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>

const double Robot::MAX_RADIUS = 0.09;

Robot::Robot(bool yellow, unsigned int pattern_index) : yellow(yellow), pattern_index(pattern_index), sign(1.0), vision_failures(0), seen_this_frame(false) {
}

Robot::ptr Robot::create(bool yellow, unsigned int pattern_index) {
	ptr p(new Robot(yellow, pattern_index));
	return p;
}

void Robot::update(const SSL_DetectionRobot &packet) {
	if (packet.has_orientation()) {
		const Point new_pos(packet.x() / 1000.0 * sign, packet.y() / 1000.0 * sign);
		const double new_ori = angle_mod(packet.orientation() + (sign > 0 ? 0.0 : M_PI));
		add_prediction_datum(new_pos, new_ori);
	} else {
		LOG_WARN("Vision packet has robot with no orientation.");
	}
}

