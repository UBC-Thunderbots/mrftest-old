#include "ai/tactic/move_between_opponents.h"

move_between_opponents::move_between_opponents(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , move_tactic(new move(ball, field, team, player)) {
	
}

void move_between_opponents::set_opponents(robot::ptr oppA, robot::ptr oppB) {
	opponentA = oppA;
	opponentB = oppB;
}

point move_between_opponents::calculate_position() {
	// TODO: Think of a better calculation
	if (!opponentA->has_ball())
		return opponentA->position();
	else
		return opponentB->position();
}

void move_between_opponents::tick() {
	move_tactic->set_position(calculate_position());
	move_tactic->tick();
}
