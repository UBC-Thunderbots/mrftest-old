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
	
	for (size_t index = 0; index < players.size(); ++index) {
		//if ((robots[index]->position() - ball).len() > 1.0 || robots[index] != the_world->friendly.get_player(closest)) {
		if (true) {
			Move tactic(players[index], the_world);
			if (players.size() == 2) {
				if (index == 0) {
					if (players[0]->position().y > players[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				} else {
					if (players[0]->position().y < players[1]->position().y) ball.y+=0.4;
					else ball.y-=0.4;
				}
			}
			Point half = (ball + the_world->field().friendly_goal()) * 0.5;
			tactic.set_position(Point(half.x, half.y));
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			Shoot tactic(players[index], the_world);
			tactic.force();
			tactic.set_flags(flags);
			tactic.tick();
		}
	}
}

void ByronsDefender::players_changed() {
}

