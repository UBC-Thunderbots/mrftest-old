#include "ai/tactic/shoot.h"
#include "ai/tactic/chase.h"
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

		/*
		std::vector<point> candidates = ai_util::calc_candidates(the_world);
		int best_point = ai_util::calc_best_shot(the_player, the_world);
		// if all the points are equally bad (opponent robot in all projections),
		// do random?
		if (best_point == -1) {
			best_point = rand() % ai_util::SHOOTING_SAMPLE_POINTS;
		}
		const point target = candidates[best_point];
		*/
		const point target = ai_util::calc_best_shot2(the_world, the_world->ball()->position());
		// std::cout << " target = " << target << std::endl;

		// shoot
		kick kick_tactic(the_player, the_world);
		kick_tactic.set_target(target);
		kick_tactic.tick();	
	} else if (ai_util::posses_ball(the_world, the_player)) {
		// std::cout << " chase ball close " << the_player->sense_ball_time() << std::endl;
		// We have the ball right but somehow it was momentarily lost.
		chase chase_tactic(the_player, the_world);
		chase_tactic.set_flags(flags);
		chase_tactic.tick();
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
			chase chase_tactic(the_player, the_world);
			chase_tactic.set_flags(flags);
			chase_tactic.tick();
		} else {
			// otherwise be ready for receive
			receive receive_tactic(the_player, the_world);
			receive_tactic.set_flags(flags);
			receive_tactic.tick();
		}
	}
}

