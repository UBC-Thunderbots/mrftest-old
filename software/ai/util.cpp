#include "ai/util.h"
#include "ai/param.h"

bool AI::Util::calc_fastest_grab_ball_dest(Point ball_pos, Point ball_vel, Point player_pos, Point &dest) {
	const double ux = ball_vel.len(); // velocity of ball

	const double v = AI::player_average_velocity;

	const Point p1 = ball_pos;

	const Point p2 = player_pos;
	const Point u = ball_vel.norm();

	const double x = (p2 - p1).dot(u);
	const double y = std::fabs((p2 - p1).cross(u));

	double a = 1 + (y * y) / (x * x);
	double b = (2 * y * y * ux) / (x * x);
	double c = (y * y * ux * ux) / (x * x) - v;

	double vx1 = (-b + std::sqrt(b * b - (4 * a * c))) / (2 * a);
	double vx2 = (-b - std::sqrt(b * b - (4 * a * c))) / (2 * a);

	double t1 = x / (vx1 + ux);
	double t2 = x / (vx2 + ux);

	double t = std::min(t1, t2);
	if (t < 0) {
		t = std::max(t1, t2);
	}

	dest = ball_pos;

	if (std::isnan(t) || std::isinf(t) || t < 0) {
		return false;
	}

	dest = p1 + ball_vel * 2 * t;

	return true;
}

