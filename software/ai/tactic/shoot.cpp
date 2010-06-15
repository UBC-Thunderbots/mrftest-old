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
	const enemy_team &enemy(the_world->enemy);

	std::vector<point> obstacles;
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get_player(i) == the_player) continue;
		obstacles.push_back(friendly.get_player(i)->position());
	}
	for (size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get_robot(i)->position());
	}

	const std::pair<point, double> bestshot = ai_util::calc_best_shot(the_world->field(), obstacles, the_player->position());

	if (ai_util::has_ball(the_player)) {
		// This player has the ball.

		std::vector<point> obstacles;
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get_player(i) == the_player) continue;
			obstacles.push_back(friendly.get_player(i)->position());
		}
		for (size_t i = 0; i < enemy.size(); ++i) {
			obstacles.push_back(enemy.get_robot(i)->position());
		}

		const point diff = bestshot.first - the_player->position();
		const double diffangle = diff.orientation();

		// calculate where to aim
		move move_tactic(the_player, the_world);
		move_tactic.set_orientation(diffangle);

		// dribble if possible to
		if (the_player->dribble_distance() < player::MAX_DRIBBLE_DIST) {
			move_tactic.set_position(bestshot.first);
		}

		std::cout << "shoot: target=" << bestshot.first << " player angle off=" << angle_diff(diffangle, the_player->orientation()) << " target tolerance=" << bestshot.second << std::endl;

		// check if the goal is within shooting range. if so, kick
		if (angle_diff(diffangle, the_player->orientation()) < bestshot.second / 2) {
			// kick realy really hard
			if (the_player->chicker_ready_time() == 0) {
				std::cout << "shoot: kick to goal" << std::endl;
				the_player->kick(1.0);
			} else {
				std::cout << "shoot: chicker not ready" << std::endl;
			}
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
		tactic.set_target(bestshot.first);
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
			tactic.set_target(bestshot.first);
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

