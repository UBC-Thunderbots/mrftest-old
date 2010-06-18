#include "ai/tactic/shoot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pivot.h"
#include "geom/angle.h"
#include "ai/util.h"

#include <vector>
#include <iostream>

shoot::shoot(player::ptr player, world::ptr world) : tactic(player), the_world(world), forced(false), use_pivot(true) {
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

#warning enable forced kicking

	const std::pair<point, double> bestshot = ai_util::calc_best_shot(the_world->field(), obstacles, the_player->position());

	if (ai_util::has_ball(the_world, the_player)) {
		// This player has the ball.

		std::vector<point> obstacles;
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get_player(i) == the_player) continue;
			obstacles.push_back(friendly[i]->position());
		}
		for (size_t i = 0; i < enemy.size(); ++i) {
			obstacles.push_back(enemy[i]->position());
		}

		const point diff = bestshot.first - the_player->position();
		const double targetori = diff.orientation();

		// calculate where to aim
		move move_tactic(the_player, the_world);
		move_tactic.set_orientation(targetori);

		// dribble if possible to
		if (the_player->dribble_distance() < player::MAX_DRIBBLE_DIST) {
			move_tactic.set_position(bestshot.first);
		}

		const double anglediff = angle_diff(targetori, the_player->orientation());
		std::cout << "shoot: target=" << bestshot.first << " player angle off=" << anglediff << " target tolerance=" << bestshot.second << std::endl;

		// check if the goal is within shooting range. if so, kick
		if (anglediff < bestshot.second / 2) {
			// kick realy really hard
			if (the_player->chicker_ready_time() == 0) {
				std::cout << "shoot: kick" << std::endl;
				the_player->kick(1.0);
			} else {
				std::cout << "shoot: chicker not ready" << std::endl;
			}
		} else {
			std::cout << "shoot: aiming" << std::endl;
		}

		move_tactic.set_flags(flags);
		move_tactic.tick();	
	} else if (use_pivot) {
		pivot tactic(the_player, the_world);
		tactic.set_target(bestshot.first);
		tactic.set_flags(flags);
		tactic.tick();
	} else {
		chase chase_tactic(the_player, the_world);
		chase_tactic.set_flags(flags);
		chase_tactic.tick();
	}
}

