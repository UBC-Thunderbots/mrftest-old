#include "ai/tactic/turn.h"

turn::turn(player::ptr player) : the_player(player) {
}

void turn::set_direction(const point& dir) {
	the_direction = dir;
}

double turn::d_angle() {
	point target = the_direction - the_player->position();
	point orient(1,0);	
	double theta = acos(orient.dot(target) / target.len());

	if (target.y < 0)
		theta = 2 * M_PI - theta;

	return theta;
}

bool turn::is_turned(const double& tol) {
	return d_angle() < tol;
}

void turn::tick()
{
	the_player->move(the_player->position(), d_angle());
}
