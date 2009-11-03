#include "ai/role.h"

using namespace std;

role::role(ball::ptr ball, field::ptr field, controlled_team::ptr team) : the_ball(ball), the_field(field), the_team(team) {
}

void role::setRobots(vector<robot::ptr> robots) {
	role::the_robots = robots;
}

void role::emptyRobots() {
	role::the_robots.empty();
}
