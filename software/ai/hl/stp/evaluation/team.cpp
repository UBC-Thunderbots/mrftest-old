#include "ai/hl/stp/evaluation/team.h"

using namespace AI::HL::STP;

Player::CPtr AI::HL::STP::Evaluation::get_nearest_friendly(const World& world, Point target) {
	int dist_i = -1;
	double dist = 0;

	for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
		double d = (target - world.friendly_team().get(i)->position()).len();
		if (dist_i < 0 || d < dist) {
			dist_i = static_cast<int>(i); 
			dist = d;
		}
	}

	return world.friendly_team().get(dist_i);
}

Robot::Ptr AI::HL::STP::Evaluation::get_nearest_enemy(const World &world, Point target) {
	int dist_i = -1;
	double dist = 0;

	for (std::size_t i = 0; i < world.enemy_team().size(); i++) {
		double d = (target - world.enemy_team().get(i)->position()).len();
		if (dist_i < 0 || d < dist) {
			dist_i = static_cast<int>(i); 
			dist = d;
		}
	}

	return world.enemy_team().get(dist_i);
}


