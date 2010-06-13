#include "ai/tactic/shoot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/receive.h"
#include "geom/angle.h"
#include "ai/util.h"

#include <vector>
#include <iostream>

shoot::shoot(player::ptr player, world::ptr world) : tactic(player), the_world(world) {
}

void shoot::tick() {
	const friendly_team &friendly(the_world->friendly);
	if (ai_util::has_ball(the_player)) {
		// This player has the ball.

		const std::pair<point, double> bestshot = ai_util::calc_best_shot2(the_world, the_world->ball()->position());
		const point diff = bestshot.first - the_player->position();
		const double diffangle = diff.orientation();

		// calculate where to aim
		move move_tactic(the_player, the_world);
		move_tactic.set_orientation(diffangle);
		move_tactic.set_position(bestshot.first);

		std::cout << " shoot pos=" << bestshot.first << " angle diff = " << angle_diff(diffangle, the_player->orientation()) << " bestshot=" << bestshot.second << std::endl;

		// check if the goal is within shooting range. if so, kick
		if (angle_diff(diffangle, the_player->orientation()) < bestshot.second / 4) {
			std::cout << "shoot: kick to goal" << std::endl;
			// kick realy really hard
			the_player->kick(1.0);
		} else {
			std::cout << "shoot: aim to goal" << std::endl;
		}

		move_tactic.set_flags(flags);
		move_tactic.tick();	
	} else if (ai_util::posses_ball(the_world, the_player)) {
		// std::cout << " chase ball close " << the_player->sense_ball_time() << std::endl;
		// We have the ball right but somehow it was momentarily lost.
		//chase chase_tactic(the_player, the_world);
		//chase_tactic.set_flags(flags);
		//chase_tactic.tick();
		pivot tactic(the_player, the_world);
		tactic.set_flags(flags);
		tactic.tick();
	} else {
		// This player does not have the ball.
		// std::cout << " omg no ball, go chase " << the_player->sense_ball() << " last=" << the_player->last_sense_ball_time() << std::endl;

		bool teampossesball = false;
		for (unsigned int i = 0; i < friendly.size(); ++i) {
			if (ai_util::posses_ball(the_world, friendly.get_player(i))) {
				teampossesball = true;
				break;
			}
		}
		if (!teampossesball) {
			// chase if our team does not have the ball
			pivot tactic(the_player, the_world);
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			// otherwise be ready for receive
			receive receive_tactic(the_player, the_world);
			receive_tactic.set_flags(flags);
			receive_tactic.tick();
		}
	}
}

