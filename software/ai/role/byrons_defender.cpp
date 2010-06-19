#include "ai/role/byrons_defender.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/util.h"
#include <iostream>

byrons_defender::byrons_defender(world::ptr world) : the_world(world) {
}

void byrons_defender::tick() {

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	
	for (size_t index = 0; index < the_robots.size(); ++index) {
		point ball = the_world->ball()->position();
		if ((the_robots[index]->position() - ball).len() > 0.4) {
			move tactic(the_robots[index], the_world);
			if (the_robots.size() == 2) {
				if (index == 0) {
					if (the_robots[0]->position().y > the_robots[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				} else {
					if (the_robots[0]->position().y < the_robots[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				}
			}
			point half = (ball + the_world->field().friendly_goal()) * 0.5;
			tactic.set_position(point(half.x, half.y));
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			shoot tactic(the_robots[index], the_world);
			tactic.set_flags(flags);
			tactic.tick();
		}
	}
}

void byrons_defender::robots_changed() {
}

