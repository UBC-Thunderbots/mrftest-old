#include "ai/role/byrons_defender.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/util.h"
#include <iostream>

ByronsDefender::ByronsDefender(World::ptr world) : the_world(world) {
}

void ByronsDefender::tick() {

	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	
	Point ball = the_world->ball()->position();
			
	int closest = 0;
	double minLength = 100;
	for (size_t i = 0; i < the_world->friendly.size(); ++i) {
		if ((the_world->friendly.get_player(i)->position() - ball).len() < minLength) {
			minLength = (the_world->friendly.get_player(i)->position() - ball).len();
			closest = i; 
		}
	}
	
	for (size_t index = 0; index < the_robots.size(); ++index) {
		//if ((the_robots[index]->position() - ball).len() > 1.0 || the_robots[index] != the_world->friendly.get_player(closest)) {
		if (true) {
			Move tactic(the_robots[index], the_world);
			if (the_robots.size() == 2) {
				if (index == 0) {
					if (the_robots[0]->position().y > the_robots[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				} else {
					if (the_robots[0]->position().y < the_robots[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				}
			}
			Point half = (ball + the_world->field().friendly_goal()) * 0.5;
			tactic.set_position(Point(half.x, half.y));
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			Shoot tactic(the_robots[index], the_world);
			tactic.force();
			tactic.set_flags(flags);
			tactic.tick();
		}
	}
}

void ByronsDefender::robots_changed() {
}

