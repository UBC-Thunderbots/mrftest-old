#include "ai/tactic/pass.h"
#include "geom/point.h"

pass::pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player, player::ptr receiver) : tactic(ball, field, team, player), kick_tactic(new kick(ball, field, team, player)) , move_tactic(new move(ball, field, team, player)) {
	the_receiver = receiver;
}

void pass::tick() {

	point path = the_receiver->position() - the_player->position();
	team::ptr opponent_team = the_team->other();
	bool should_wait = false;
	
	double projection, dist;
	for (unsigned int i = 0; i < opponent_team->size(); ++i)
	{
		robot::ptr other_robot = opponent_team->get_robot(i);
		point other = other_robot->position() - the_player->position();
		
		projection = other.dot(path.norm());
		dist = sqrt(other.lensq() - projection * projection);

		// opponent robot in the passing lane
		if (dist <= robot::MAX_RADIUS + INTERCEPT_RADIUS)
		{
			should_wait = true;
			break;
		}
	}
	// Also check the passee is not moving
	should_wait = should_wait || (the_receiver->est_velocity().len() > SPEED_THRESHOLD);

	// only kick the ball to the receiver if the passing lane is clear, otherwise, move towards the receiver
	if (should_wait)
	{
		move_tactic->set_position(the_receiver->position());
		move_tactic->tick();
	}
	else
	{
		kick_tactic->set_target(the_receiver->position());
		kick_tactic->tick();
	}
}


