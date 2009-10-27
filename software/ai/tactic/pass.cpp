#include "ai/tactic/pass.h"

pass::pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), kick_tactic(new kick(ball, field, team, player)) {
}

void pass::set_receiver(player::ptr receiver) {
	the_receiver = receiver;
}		

void pass::update() {
	kick_tactic->set_target(calculate_target());
	kick_tactic->update();
}

point pass::calculate_target() {	
	switch (mode) {
		case STATIONARY:
			// assume the receiver is stationary
			return the_receiver->position();
		case FULL_SPEED:
			// assuming the receiver is moving full speed forward
			// TODO: use velocity of the ball
			return the_receiver->position() + the_receiver->est_velocity();
		default:
			return the_receiver->position();
		break;
	}
	return point(0,0);
}


