#include "ai/role.h"

role::role(ball::ptr ball, field::ptr field, controlled_team::ptr team) : the_ball(ball), the_field(field), the_team(team) {
}

void role::set_robots(const std::vector<player::ptr> &robots) {
	the_robots = robots;
}

void role::clear_robots() {
	the_robots.clear();
}

