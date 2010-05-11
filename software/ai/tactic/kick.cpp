#include "ai/tactic/kick.h"
#include <iostream>

kick::kick(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), turn_tactic(new turn(ball, field, team, player)), should_chip(true) {
}

void kick::set_target(const point& p) {
	the_target = p;
}

void kick::set_chip(const bool& chip) {
	should_chip = chip;
}

void kick::tick() {
	// turn towards the target
	turn_tactic->set_direction(the_target);

	const double TOL = 10;

	if (!turn_tactic->is_turned(TOL)) 
		turn_tactic->tick();	
	else {
//		std::cout << "kicking" << std::endl;
		// assume maximum strength for now...
		if (should_chip){
			if (the_player->has_ball()){
				the_player->chip(1);
			}
		}
			
		else{
			if (the_player->has_ball()){
				the_player->kick(1);
			}
		}
			
	}

}

